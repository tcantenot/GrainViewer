/**
 * This file is part of GrainViewer, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2020).
 *   Real Time Multiscale Rendering of Dense Dynamic Stackings,
 *   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
 *   https://doi.org/10.1111/cgf.14135
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

#include "ImpostorGrainRenderer.h"
#include "TransformBehavior.h"
#include "ShaderPool.h"
#include "BehaviorRegistry.h"
#include "GrainBehavior.h"
#include "ResourceManager.h"
#include "utils/ScopedFramebufferOverride.h"
#include "GlobalTimer.h"
#include "utils/impostor.glsl.h"

#include "utils/jsonutils.h"
#include "utils/behaviorutils.h"

//-----------------------------------------------------------------------------

const std::vector<std::string> ImpostorGrainRenderer::s_shaderVariantDefines = {
	"NO_DISCARD",
	"PASS_SHADOW_MAP",
	"PASS_BLIT_TO_MAIN_FBO",
	"NO_INTERPOLATION",
	"PRECOMPUTE_IMPOSTOR_VIEW_MATRICES",
	"PRECOMPUTE_IN_VERTEX",
};

bool ImpostorGrainRenderer::deserialize(const rapidjson::Value & json)
{
	jrOption(json, "shader", m_shaderName, m_shaderName);

	std::string colormap;
	if (jrOption(json, "colormap", colormap)) {
		m_colormapTexture = ResourceManager::loadTexture(colormap);
	}

	autoDeserialize(json, m_properties);

	return true;
}

void ImpostorGrainRenderer::start()
{
	m_transform = getComponent<TransformBehavior>();
	m_grain = getComponent<GrainBehavior>();
	m_pointData = BehaviorRegistry::getPointCloudDataComponent(*this, PointCloudSplitter::RenderModel::Impostor);
	m_splitter = getComponent<PointCloudSplitter>();
}

void ImpostorGrainRenderer::update(float time, int frame)
{
	m_time = time;
	if (properties().precomputeViewMatrices && !m_precomputedViewMatrices) {
		precomputeViewMatrices();
	}
}

void ImpostorGrainRenderer::render(const Camera& camera, const World& world, RenderType target) const
{
	ScopedTimer timer((target == RenderType::ShadowMap ? "ImpostorGrainRenderer_shadowmap" : "ImpostorGrainRenderer"));

	auto pointData = m_pointData.lock();
	if (!pointData) return;

	ScopedFramebufferOverride scoppedFramebufferOverride; // to automatically restore fbo binding at the end of scope
	const Properties& props = properties();

	glEnable(GL_PROGRAM_POINT_SIZE);

	std::shared_ptr<Framebuffer> fbo;
	if (props.noDiscard && target != RenderType::ShadowMap) {
		// If not using discards in main draw call, we render in a separate
		// framebuffer and only then blit it onto the main framebuffer
		fbo = camera.getExtraFramebuffer(Camera::ExtraFramebufferOption::LinearGBufferDepth);
		fbo->bind();
	}

	// 1. Clear depth
	if (fbo) {
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	// 2. Main drawing, cumulativly if there is an extra fbo
	{
		glDepthMask(GL_TRUE);
		if (fbo) {
			glDepthFunc(GL_ALWAYS);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
		}
		else {
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
		}

		// Get shader
		ShaderVariantFlagSet flags = 0;
		if (target == RenderType::ShadowMap) flags |= ShaderPassShadow;
		if (props.noDiscard) flags |= ShaderOptionNoDiscard;
		if (props.precomputeViewMatrices) flags |= ShaderOptionPrecomputeViewMatrices;
		if (props.precomputeInVertex) flags |= ShaderOptionPrecomputeInVertex;
		if (props.interpolationMode == InterpolationMode::None) flags |= ShaderOptionNoInterpolation;
		const ShaderProgram& shader = *getShader(flags);

		setCommonUniforms(shader, camera);
		shader.setUniform("uPrerenderSurfaceStep", 0);
		draw(*pointData, shader);

		if (props.prerenderSurface && !props.firstPassOnly) {
			setCommonUniforms(shader, camera);
			shader.setUniform("uPrerenderSurfaceStep", 1);
			draw(*pointData, shader);
		}
	}

	// 3. Blit auxilliary fbo to main fbo
	if (fbo) {
		scoppedFramebufferOverride.restore();

		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		// Get shader
		ShaderVariantFlagSet flags = ShaderPassBlitToMainFbo;
		if (target == RenderType::ShadowMap) flags |= ShaderPassShadow;
		if (props.noDiscard) flags |= ShaderOptionNoDiscard;
		if (props.precomputeViewMatrices) flags |= ShaderOptionPrecomputeViewMatrices;
		if (props.precomputeInVertex) flags |= ShaderOptionPrecomputeInVertex;
		if (props.interpolationMode == InterpolationMode::None) flags |= ShaderOptionNoInterpolation;
		const ShaderProgram& shader = *getShader(flags);

		// Set uniforms

		// Bind secondary FBO textures
		glTextureBarrier();
		GLint o = 0;
		for (int i = 0; i < fbo->colorTextureCount(); ++i) {
			glBindTextureUnit(static_cast<GLuint>(o), fbo->colorTexture(i));
			shader.setUniform(MAKE_STR("lgbuffer" << i), o);
			++o;
		}
		glBindTextureUnit(static_cast<GLuint>(o), fbo->depthTexture());
		shader.setUniform("uFboDepthTexture", o);
		++o;

		shader.use();
		PostEffect::DrawWithDepthTest();
	}
}


//-----------------------------------------------------------------------------

void ImpostorGrainRenderer::draw(const IPointCloudData& pointData, const ShaderProgram& shader) const
{
	// Draw call
	shader.use();
	glBindVertexArray(pointData.vao());
	if (auto ebo = pointData.ebo()) {
		pointData.vbo().bindSsbo(0);
		ebo->bindSsbo(1);
		shader.setUniform("uUsePointElements", true);
	}
	else {
		shader.setUniform("uUsePointElements", false);
	}
	glDrawArrays(GL_POINTS, pointData.pointOffset(), pointData.pointCount());
	glBindVertexArray(0);
}

void ImpostorGrainRenderer::setCommonUniforms(const ShaderProgram& shader, const Camera& camera) const
{
	const Properties& props = properties();

	// Set uniforms
	glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
	shader.bindUniformBlock("Camera", camera.ubo());
	shader.setUniform("modelMatrix", modelMatrix());
	shader.setUniform("viewModelMatrix", viewModelMatrix);

	autoSetUniforms(shader, properties());
	if (auto grain = m_grain.lock()) {
		autoSetUniforms(shader, grain->properties());
	}

	auto pointData = m_pointData.lock();
	shader.setUniform("uPointCount", static_cast<GLuint>(pointData->pointCount()));
	shader.setUniform("uFrameCount", static_cast<GLuint>(pointData->frameCount()));
	shader.setUniform("uTime", static_cast<GLfloat>(m_time));

	GLint o = 0;

	if (m_colormapTexture) {
		m_colormapTexture->bind(o);
		shader.setUniform("uColormapTexture", o++);
	}

	if (auto grain = m_grain.lock()) {
		for (size_t k = 0; k < grain->atlases().size(); ++k) {
			o = grain->atlases()[k].setUniforms(shader, MAKE_STR("uImpostor[" << k << "]."), o);
		}
	}

	if (props.precomputeViewMatrices) {
		m_precomputedViewMatrices->bindSsbo(4);
	}

	shader.setUniform("uUseOcclusionMap", false);
	if (auto splitter = m_splitter.lock()) {
		if (splitter->properties().enableOcclusionCulling) {
			// This is a hack: we reuse the fbo that was used by the splitter and assume nothing else has written to it in the meantime
			auto occlusionCullingFbo = camera.getExtraFramebuffer(Camera::ExtraFramebufferOption::Rgba32fDepth);
			glBindTextureUnit(static_cast<GLuint>(o), occlusionCullingFbo->colorTexture(0));
			shader.setUniform("uOcclusionMap", o++);
			shader.setUniform("uUseOcclusionMap", true);
		}
	}
}

void ImpostorGrainRenderer::precomputeViewMatrices()
{
	auto grain = m_grain.lock();
	if (!grain) return;

	m_precomputedViewMatrices = std::make_unique<GlBuffer>(GL_SHADER_STORAGE_BUFFER);
	// Check that viewCount is the same for all impostors
	GLuint n = grain->atlases()[0].viewCount;
	for (int i = 1 ; i < grain->atlases().size() ; ++i) {
		if (grain->atlases()[i].viewCount != n) {
			ERR_LOG << "Precomputed view matrices can only be used when all impostors use the same number of views";
			properties().precomputeViewMatrices = false;
			return;
		}
	}
	m_precomputedViewMatrices->addBlock<glm::mat4>(static_cast<size_t>(2 * n * n));
	m_precomputedViewMatrices->alloc();
	m_precomputedViewMatrices->fillBlock<glm::mat4>(0, [n](glm::mat4 *data, size_t size) {
		for (glm::uint i = 0; i < size; ++i) {
			data[i] = glsl::InverseBakingViewMatrix(i, n);
		}
	});
	m_precomputedViewMatrices->finalize();
}

glm::mat4 ImpostorGrainRenderer::modelMatrix() const
{
	if (auto transform = m_transform.lock()) {
		return transform->modelMatrix();
	} else {
		return glm::mat4(1.0f);
	}
}

std::shared_ptr<ShaderProgram> ImpostorGrainRenderer::getShader(ShaderVariantFlagSet flags) const
{
	constexpr int nFlags = static_cast<int>(magic_enum::enum_count<ShaderVariantFlag>());
	if (m_shaders.empty()) {
		m_shaders.resize(1 << nFlags);
	}

	if (!m_shaders[flags]) {
		// Lazy loading of shader variants
		std::string variantName = m_shaderName + "_ShaderVariantFlags_" + bitname(flags, nFlags);
		std::vector<std::string> defines;
		for (int f = 0; f < nFlags; ++f) {
			if ((flags & (1 << f)) != 0) {
				defines.push_back(s_shaderVariantDefines[f]);
			}
		}
		DEBUG_LOG << "loading variant " << variantName;
		ShaderPool::AddShaderVariant(variantName, m_shaderName, defines);
		m_shaders[flags] = ShaderPool::GetShader(variantName);
	}
	return m_shaders[flags];
}

