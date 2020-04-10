#pragma once

#include <OpenGL>

#include "Behavior.h"
#include "GlBuffer.h"
#include "IPointCloudData.h"
#include "utils/ReflectionAttributes.h"

#include <refl.hpp>

#include <memory>
#include <vector>

class ShaderProgram;
class PointCloudView;
class TransformBehavior;
class SandBehavior;

/**
 * The Point Cloud Splitter behavior uses the preRender pass to split
 * the point cloud into contiguous element buffers for each rendering model.
 * This component must be placed *after* point data.
 */
class PointCloudSplitter : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void update(float time, int frame) override;
	void onPreRender(const Camera& camera, const World& world, RenderType target) override;

public:
	enum class RenderTypeCaching {
		Forget, // Uses less memory
		Cache, // Faster, but by max 1%...
		Precompute, // Not recommended
	};
	struct Properties {
		RenderTypeCaching renderTypeCaching = RenderTypeCaching::Forget;
		bool enableOcclusionCulling = true;
		bool enableFrustumCulling = true;
		float instanceLimit = 1.05f; // distance beyond which we switch from instances to impostors
		float impostorLimit = 10.0f;
		bool zPrepass = true; // for occluder map
		bool useBbox = false; // if true, remove all points out of the supplied bounding box
		glm::vec3 bboxMin;
		glm::vec3 bboxMax;
		float occluderMapSpriteScale = 0.2f;
	};
	enum class RenderModel {
		Instance = 0,
		Impostor,
		Point,
		None,
	};

	Properties & properties() { return m_properties; }
	const Properties& properties() const { return m_properties; }

	struct Counter {
		GLuint count = 0;
		GLuint offset = 0;
	};
	const std::vector<Counter> counters() const { return m_counters; }

	// Return a point buffer for a given model
	std::shared_ptr<PointCloudView> subPointCloud(RenderModel model) const;
	GLsizei pointCount(RenderModel model) const;
	GLsizei frameCount(RenderModel model) const;
	GLuint vao(RenderModel model) const;
	const GlBuffer& vbo(RenderModel model) const;
	std::shared_ptr<GlBuffer> ebo(RenderModel model) const;
	GLint pointOffset(RenderModel model) const;

private:
	glm::mat4 modelMatrix() const;
	void setCommonUniforms(const ShaderProgram& shader, const Camera& camera) const;

	// These must match defines in the shader (magic_enum reflexion is used to set defines)
	// the first one mirrors RenderTypeCaching (which is for diaplay)
	enum class RenderTypeShaderVariant {
		RENDER_TYPE_FORGET,
		RENDER_TYPE_CACHE,
		RENDER_TYPE_PRECOMPUTE,
	};
	enum class StepShaderVariant {
		STEP_PRECOMPUTE,
		STEP_RESET,
		STEP_COUNT,
		STEP_OFFSET,
		STEP_WRITE,
	};
	typedef int ShaderVariantFlagSet;
	std::shared_ptr<ShaderProgram> getShader(RenderTypeCaching renderType, int step) const; // for convenience
	std::shared_ptr<ShaderProgram> getShader(RenderTypeShaderVariant renderType, StepShaderVariant step) const;

private:
	Properties m_properties;

	std::string m_shaderName = "GlobalAtomic";
	std::string m_occlusionCullingShaderName = "OcclusionCulling";
	mutable std::vector<std::shared_ptr<ShaderProgram>> m_shaders; // mutable for lazy loading, do NOT use this directly, rather use getShader()
	std::shared_ptr<ShaderProgram> m_occlusionCullingShader;

	std::weak_ptr<TransformBehavior> m_transform;
	std::weak_ptr<SandBehavior> m_sand;
	std::weak_ptr<IPointCloudData> m_pointData;

	std::shared_ptr<GlBuffer> m_elementBuffer; // must be shared because exposed through IPointCloudData interface
	mutable std::unique_ptr<GlBuffer> m_renderTypeCache; // lazily allocated

	std::vector<Counter> m_counters;
	std::unique_ptr<GlBuffer> m_countersSsbo;

	// Output subclouds
	std::vector<std::shared_ptr<PointCloudView>> m_subClouds;

	GLuint m_elementCount;
	int m_local_size_x = 128;
	int m_xWorkGroups;
	float m_time;
};

#define _ ReflectionAttributes::
REFL_TYPE(PointCloudSplitter::Properties)
REFL_FIELD(renderTypeCaching)
REFL_FIELD(enableOcclusionCulling)
REFL_FIELD(enableFrustumCulling)
REFL_FIELD(instanceLimit, _ Range(0.01f, 3.0f))
REFL_FIELD(impostorLimit, _ Range(0.01f, 20.0f))
REFL_FIELD(zPrepass)
REFL_FIELD(useBbox)
REFL_FIELD(bboxMin, _ Range(-1, 1))
REFL_FIELD(bboxMax, _ Range(-1, 1))
REFL_FIELD(occluderMapSpriteScale)
REFL_END
#undef _

registerBehaviorType(PointCloudSplitter)

//-----------------------------------------------------------------------------

/**
 * Proxy to an externally allocated element buffer,
 * used by PointCloudSplitter to return sub parts of the original point cloud.
 * This is technically just a closure around PointCloudSplitter's method that
 * are a bit like IPointCloudData but with an extra model parameter.
 */
class PointCloudView : public IPointCloudData {
public:
	PointCloudView(const PointCloudSplitter & splitter, PointCloudSplitter::RenderModel model)
		: m_splitter(splitter), m_model(model) {}
	// IPointCloudData implementation
	GLsizei pointCount() const override { return m_splitter.pointCount(m_model); }
	GLsizei frameCount() const override { return m_splitter.frameCount(m_model); }
	GLuint vao() const override { return m_splitter.vao(m_model); }
	const GlBuffer& vbo() const override { return m_splitter.vbo(m_model); }
	std::shared_ptr<GlBuffer> ebo() const override { return m_splitter.ebo(m_model); }
	GLint pointOffset() const override { return m_splitter.pointOffset(m_model); }

private:
	const PointCloudSplitter & m_splitter;
	PointCloudSplitter::RenderModel m_model;
};

