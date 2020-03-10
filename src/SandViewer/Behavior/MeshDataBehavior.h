#pragma once

#ifdef _WIN32
#include <windows.h> // Avoid issue with APIENTRY redefinition in Glad
#endif // _WIN32

#include <glad/modernglad.h>

#include <memory>
#include <glm/glm.hpp>

#include "Behavior.h"
#include "Mesh.h"
#include "GlBuffer.h"

/**
 * Load mesh from OBJ file to video memory
 */
class MeshDataBehavior : public Behavior {
public:
	// Accessors
	GLsizei pointCount() const;
	GLuint vao() const;

	// Must be called *before* start()
	void setFilename(const std::string & filename) { m_filename = filename; }

public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void onDestroy() override;

private:
	std::string m_filename = "";
	std::unique_ptr<Mesh> m_mesh;

	GLsizei m_pointCount;
	std::unique_ptr<GlBuffer> m_vertexBuffer;
	GLuint m_vao;
};

registerBehaviorType(MeshDataBehavior)

