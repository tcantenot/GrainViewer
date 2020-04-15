#version 450 core
#include "sys:defines"

#pragma variant PROCEDURAL_BASECOLOR

in VertexData {
    vec3 normal_ws;
    vec3 position_ws;
    vec3 tangent_ws;
    vec2 uv;
    flat uint materialId;
    vec3 baseColor;
} vert;

#define OUT_GBUFFER
#include "include/gbuffer2.inc.glsl"

#include "include/standard-material.inc.glsl"
uniform StandardMaterial uMaterial[3];

uniform float uNormalMapping = 0.5;

uniform bool uDebugRenderType = false;
uniform vec3 uDebugRenderColor = vec3(172.0/255.0, 23.0/255.0, 1.0/255.0);

#include "include/random.inc.glsl"
#include "sand/procedural-color.inc.glsl"

void main() {
    SurfacePoint surf = SurfacePoint(
        vert.position_ws,
        vert.normal_ws,
        vert.tangent_ws,
        vert.uv,
        uNormalMapping
    );

    GFragment fragment = SampleStandardMaterial(uMaterial[vert.materialId], surf);

    if (uDebugRenderType) {
        fragment.baseColor = uDebugRenderColor;
    } else if (isUsingProceduralColor()) {
        fragment.baseColor = vert.baseColor.rgb;
    }

    autoPackGFragment(fragment);
}

