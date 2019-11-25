#version 450 core
#include "sys:defines"

layout (points) in;
layout (points, max_vertices = 1) out;

flat in uint pointId[];

flat out uint id;
out vec3 position_ws;
out mat4 gs_from_ws;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

#include "include/utils.inc.glsl"
#include "include/random.inc.glsl"
#include "sand/random-grains.inc.glsl"

void main() {
	id = pointId[0];
	float actualRadius = 0.005;

    position_ws = (modelMatrix * gl_in[0].gl_Position).xyz;
    vec4 position_cs = viewMatrix * vec4(position_ws, 1.0);

    gl_Position = projectionMatrix * position_cs;
    
    // Estimate projection of sphere on screen to determine sprite size
    gl_PointSize = 1.0 * max(resolution.x, resolution.y) * projectionMatrix[1][1] * actualRadius / gl_Position.w;
    if (isOrthographic(projectionMatrix)) {
        float a = projectionMatrix[0][0] * resolution.x;
        float c = projectionMatrix[1][1] * resolution.y;
        gl_PointSize = max(a * actualRadius, c * actualRadius);
    }

    gs_from_ws = randomGrainMatrix(int(id), position_ws);

    EmitVertex();
    EndPrimitive();
}
