#pragma once

#include <memory>
#include <string>
#include "Behavior.h"

class TransformBehavior;
class ShaderProgram;
class PointCloudDataBehavior;
class GlTexture;

/**
 * A sand renderer focused on far grains, not implementing the closer models
 * namely impostors and instances. It does epsilon-depth-testing
 */
class FarSandRenderer : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void render(const Camera & camera, const World & world, RenderType target) const override;

public:
	// Public properties
	enum DebugShape {
		DebugShapeNone = -1,
		DebugShapeLitSphere = 0,
		DebugShapeDisc = 1,
		DebugShapeSquare = 2
	};
	enum WeightMode {
		WeightModeNone = -1,
		WeightModeLinear = 0,
		WeightModeQuad = 1,
		WeightModeGaussian = 2,
	};
	struct Properties {
		float radius = 0.007f;
		float epsilonFactor = 10.0f; // multiplied by radius
		bool useShellCulling = true;
		DebugShape debugShape = DebugShapeDisc;
		WeightMode weightMode = WeightModeNone;
	};
	Properties & properties() { return m_properties; }
	const Properties & properties() const { return m_properties; }

private:
	glm::mat4 modelMatrix() const;
	void setCommonUniforms(ShaderProgram & shader, const Camera & camera) const;

public:
	std::string m_shaderName = "FarSand";
	std::string m_epsilonZBufferShaderName = "FarSandEpsilonZBuffer";
	std::string m_colormapTextureName = "";
	Properties m_properties;

	std::shared_ptr<ShaderProgram> m_shader;
	std::shared_ptr<ShaderProgram> m_epsilonZBufferShader;
	std::weak_ptr<TransformBehavior> m_transform;
	std::weak_ptr<PointCloudDataBehavior> m_pointData;
	std::unique_ptr<GlTexture> m_colormapTexture;

	std::shared_ptr<Framebuffer> m_depthFbo;
};

registerBehaviorType(FarSandRenderer)
