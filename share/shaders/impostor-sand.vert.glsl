#version 430 core

struct PointCloundVboEntry {
    vec4 position;
};
layout(std430, binding = 0) buffer pointsSsbo {
    PointCloundVboEntry pointVertexAttributes[];
};
layout (std430, binding = 1) buffer pointElementsSsbo {
    uint pointElements[];
};

out VertexData {
	flat uint id;
} vert;

uniform uint uFrameCount = 1;
uniform uint uPointCount;
uniform float uFps = 25.0;
uniform float uTime;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

uniform bool uUseAnimation = true;
uniform bool uUsePointElements = true;

#include "include/anim.inc.glsl"

void main() {
	vert.id =
        uUsePointElements
        ? pointElements[gl_VertexID]
        : gl_VertexID;

    uint animPointId =
        uUseAnimation
        ? AnimatedPointId2(vert.id, uFrameCount, uPointCount, uTime, uFps)
        : vert.id;

	vec3 p = pointVertexAttributes[animPointId].position.xyz;

    gl_Position = vec4(p, 1.0);
}
