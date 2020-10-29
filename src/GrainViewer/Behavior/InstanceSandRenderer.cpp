/**
 * This file is part of GrainViewer
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

#include "InstanceSandRenderer.h"
#include "TransformBehavior.h"
#include "MeshDataBehavior.h"
#include "SandBehavior.h"
#include "IPointCloudData.h"
#include "ShaderPool.h"
#include "ResourceManager.h"
#include "BehaviorRegistry.h"
#include "GlobalTimer.h"

#include "utils/jsonutils.h"
#include "utils/behaviorutils.h"

bool InstanceSandRenderer::deserialize(const rapidjson::Value & json)
{
	jrOption(json, "shader", m_shaderName, m_shaderName);
	std::string colormap;
	if (jrOption(json, "colormap", colormap)) {
		m_colormapTexture = ResourceManager::loadTexture(colormap);
	}
	jrArray(json, "materials", m_materials);
	autoDeserialize(json, m_properties);
	return true;
}

void InstanceSandRenderer::start()
{
	m_transform = getComponent<TransformBehavior>();
	m_sand = getComponent<SandBehavior>();
	m_mesh = getComponent<MeshDataBehavior>();
	m_pointData = BehaviorRegistry::getPointCloudDataComponent(*this, PointCloudSplitter::RenderModel::Instance);

	m_shader = ShaderPool::GetShader(m_shaderName);
}

void InstanceSandRenderer::update(float time, int frame)
{
	m_time = time;
}

void InstanceSandRenderer::render(const Camera& camera, const World& world, RenderType target) const
{
	ScopedTimer timer((target == RenderType::ShadowMap ? "InstanceSandRenderer_shadowmap" : "InstanceSandRenderer"));

	auto mesh = m_mesh.lock();
	auto pointData = m_pointData.lock();
	if (!mesh || !pointData || pointData->pointCount() == 0) return;

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	const ShaderProgram & shader = *m_shader; // maybe switch shader depending on option/render target

	glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
	shader.bindUniformBlock("Camera", camera.ubo());
	shader.setUniform("modelMatrix", modelMatrix());
	shader.setUniform("viewModelMatrix", viewModelMatrix);

	autoSetUniforms(shader, properties());
	if (auto sand = m_sand.lock()) {
		autoSetUniforms(shader, sand->properties());
	}

	shader.setUniform("uPointCount", static_cast<GLuint>(pointData->pointCount()));
	shader.setUniform("uFrameCount", static_cast<GLuint>(pointData->frameCount()));
	shader.setUniform("uTime", static_cast<GLfloat>(m_time));

	GLint o = 0;
	if (m_colormapTexture) {
		m_colormapTexture->bind(o);
		shader.setUniform("uColormapTexture", o++);
	}

	int n = static_cast<int>(std::max(mesh->materials().size(), m_materials.size()));
	for (int i = 0; i < n; ++i) {
		const StandardMaterial& mat = i < m_materials.size() ? m_materials[i] : mesh->materials()[i];
		o = mat.setUniforms(*m_shader, MAKE_STR("uMaterial[" << i << "]."), o);
	}

	shader.use();

	glBindVertexArray(mesh->vao());
	pointData->vbo().bindSsbo(0);
	if (auto pointElements = pointData->ebo()) {
		pointElements->bindSsbo(1);
		shader.setUniform("uUsePointElements", true);
	}
	else {
		shader.setUniform("uUsePointElements", false);
	}
	glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, mesh->pointCount(), pointData->pointCount(), pointData->pointOffset());

	glBindVertexArray(0);
}


//-----------------------------------------------------------------------------

glm::mat4 InstanceSandRenderer::modelMatrix() const
{
	if (auto transform = m_transform.lock()) {
		return transform->modelMatrix();
	} else {
		return glm::mat4(1.0f);
	}
}

