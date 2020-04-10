#version 450 core
#include "sys:defines"

in GeometryData {
    vec3 position_ws;
    vec3 normal_ws;
    vec3 tangent_ws;
    vec2 uv;
    flat uint materialId;
} geo;


#define OUT_GBUFFER
#include "include/gbuffer2.inc.glsl"

#include "include/standard-material.inc.glsl"
uniform StandardMaterial uMaterial[3];

uniform float uNormalMapping = 1.0;

void main() {
    SurfacePoint surf = SurfacePoint(
        geo.position_ws,
        geo.normal_ws,
        geo.tangent_ws,
        geo.uv,
        uNormalMapping
    );

    GFragment fragment = SampleStandardMaterial(uMaterial[geo.materialId], surf);

    autoPackGFragment(fragment);
}