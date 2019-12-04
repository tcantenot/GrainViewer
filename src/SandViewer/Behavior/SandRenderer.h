#pragma once

#include <memory>
#include "Behavior.h"
#include "PointCloud.h"
#include "GlTexture.h"
#include "GlBuffer.h"

class ShaderProgram;
class MeshDataBehavior;
class TransformBehavior;
class Framebuffer;

/**
 * Sand renderer mixes impostor cloud and instance cloud using a culling
 * mechanism that discriminate points into one or the other representation
 * depending on their proximity to the view point.
 * It is a playground for developping the paper about multiscale sand rendering.
 */
class SandRenderer : public Behavior {
public:
	void initShader(ShaderProgram & shader);
	void updateShader(ShaderProgram & shader, float time);

public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void onDestroy() override;
	void update(float time) override;
	void render(const Camera & camera, const World & world, RenderType target) const override;
	void reloadShaders() override;

public:
	/**
	 * The culling step is responsible for building the element buffers used to
	 * call the impostor and intance based rendering. These buffer can be built
	 * using different mechanisms.
	 */
	enum CullingMechanism {
		/**
		 * The simplest mechanism, that accumulate original points in element
		 * buffers using global cursors atomically incremented when adding a
		 * point to the queue of instances or impostors. This global atomic
		 * might well be a problem for performances.
		 */
		AtomicSum,

		/**
		 * The element buffer is built using a more efficient summing process
		 * based on a parallel prefix sum algorithm.
		 */
		PrefixSum,

		/**
		 * Rather than sorting elements, buffer[i] = i or some special value
		 * that makes the subsequent render pipeline ignore it, the restart
		 * primitive. This works well for impostor rendering, but the instance
		 * rendering process is not compatible with this strategy, so don't use
		 * it (I keep it here because I implemented it before realizing it*
		 * would not work, and it might become useful if the instance rendering
		 * pipeline changes to something else).
		 */
		RestartPrimitive,
	};
	// Public properties
	struct Properties {
		float grainRadius = 0.007f;
		float grainInnerRadiusRatio = 0.8f; // as a ratio of grainRadius
		float grainMeshScale = 0.45f;
		float instanceLimit = 1.05f;
		bool disableImpostors = false;
		bool disableInstances = false;
		bool enableOcclusionCulling = true;
		bool enableDistanceCulling = true;
		bool enableFrustumCulling = true;
	};
	Properties & properties() { return m_properties; }
	const Properties & properties() const { return m_properties; }

	struct RenderInfo {
		int instanceCount;
		int impostorCount;
	};
	const RenderInfo & renderInfo() const { return m_renderInfo; }

private:
	glm::mat4 modelMatrix() const;

	bool load(const PointCloud & pointCloud);
	/**
	* @param textureDirectory Directory containing 2*n*n textures
	* corresponding to the views rendered by script make_octaedron.py with
	* parameter n. The number of files in the directory is used, as well as
	* their alphabetical order.
	*/
	bool loadImpostorTexture(std::vector<std::unique_ptr<GlTexture>> & textures, const std::string & textureDirectory);
	bool loadColormapTexture(const std::string & filename);

	void renderDefault(const Camera & camera, const World & world) const;
	void renderCulling(const Camera & camera, const World & world) const;
	void renderCullingPrefixSum(const Camera & camera, const World & world) const;
	void renderImpostorsDefault(const Camera & camera, const World & world) const;
	void renderInstancesDefault(const Camera & camera, const World & world) const;

private:
	struct PointersSsbo {
		GLint nextInstanceElement;
		GLint nextImpostorElement;
		GLint _pad0[2];
	};
	struct PrefixSumInfoSsbo {
		GLuint instanceCount;
		GLuint impostorCount;
		// Keep culling flag for last elements, because the prefix sum discards them
		GLuint isLastPointInstance;
		GLuint isLastPointImpostor;
	};
	enum PrefixSumCullingShaders {
		BaseShader,
		MarkImpostors,
		MarkInstances,
		Group,
		BuildCommandBuffer,
		_PrefixSumCullingShadersCount
	};

private:
	static int prefixSum(const GlBuffer & buffer0, const GlBuffer & buffer1, int elementCount, const ShaderProgram & shader);

private:
	Properties m_properties;
	mutable RenderInfo m_renderInfo;

	std::string m_shaderName = "ImpostorCloud";
	std::vector<std::string> m_cullingShaderNames;
	std::string m_instanceCloudShaderName = "InstanceCloud";
	std::string m_prefixSumShaderName = "PrefixSum";
	std::string m_occlusionCullingShaderName = "SandOcclusionCulling";

	std::string m_shadowMapShaderName;

	CullingMechanism m_cullingMechanism = AtomicSum;

	std::shared_ptr<ShaderProgram> m_shader;
	std::shared_ptr<ShaderProgram> m_shadowMapShader;
	std::vector<std::shared_ptr<ShaderProgram>> m_cullingShaders;
	std::shared_ptr<ShaderProgram> m_instanceCloudShader;
	std::shared_ptr<ShaderProgram> m_prefixSumShader;
	std::shared_ptr<ShaderProgram> m_occlusionCullingShader;
	std::unique_ptr<Framebuffer> m_occlusionCullingMap; // kind of shadow map
	bool m_isDeferredRendered;
	size_t m_nbPoints;
	size_t m_frameCount;

	GLuint m_vao;
	GLuint m_vbo; // TODO: replace this with GlBuffer
	std::unique_ptr<GlBuffer> m_commandBuffer;
	std::unique_ptr<GlBuffer> m_cullingPointersSsbo;
	std::unique_ptr<GlBuffer> m_prefixSumInfoSsbo;
	std::vector < std::unique_ptr<GlBuffer>> m_elementBuffers;

	std::weak_ptr<TransformBehavior> m_transform;
	std::weak_ptr<MeshDataBehavior> m_grainMeshData;

	std::vector<std::unique_ptr<GlTexture>> m_normalAlphaTextures;
	std::vector<std::unique_ptr<GlTexture>> m_baseColorTextures;
	std::vector<std::unique_ptr<GlTexture>> m_metallicRoughnessTextures;
	std::unique_ptr<GlTexture> m_colormapTexture;
};

registerBehaviorType(SandRenderer)
