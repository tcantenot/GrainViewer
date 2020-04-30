#version 450 core
#include "sys:defines"

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (rgba8, binding = 0) uniform writeonly image2D image;

uniform sampler2D uInputTexture;
uniform float uTileSize = 150;
uniform int uBlendMode = 1;
uniform float uRandomTranslation = 1.0;
uniform float uRandomRotation = 1.0;
uniform bool uContrastRestoration = true;
const float sqrt3 = sqrt(3);

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec2 randUv(vec2 co){
    float r = rand(co);
    float r2 = rand(vec2(co.x - r * 0.5123, co.y + r));
    return vec2(r, r2);
}

mat2 randRot(vec2 co) {
	float th = (rand(co + vec2(12.3, 0.6541)) - 0.5) * 2. * 3.141592 * uRandomRotation;
	float c = cos(th), s = sin(th);
	return mat2(c, s, -s, c);
}

void main() {
	// accumulate	
	ivec2 co = ivec3(gl_GlobalInvocationID.xyz).xy;
	ivec3 size = ivec3(imageSize(image), 1);
	if (co.x >= size.x || co.y >= size.y) return;

	mat2 invm = (1.0/uTileSize) * mat2(
		1, 0,
		-sqrt3/3, 2*sqrt3/3
	);
	//invm = (1.0/uTileSize) * mat2(1.0);
	//mat2 m = uTileSize * mat2(
	//	1, 0,
	//	1.0/2.0, sqrt3/2
	//);
	mat2 m = inverse(invm);

	// convert pixel pos to ab coordinates (hex grid)
	vec2 xy = vec2(co);
	vec2 ab = invm * xy;

	// cps are control vertices in ab coordinates
	vec2 cp0 = vec2(floor(ab.x), ceil(ab.y));
	vec2 cp1 = vec2(ceil(ab.x), floor(ab.y));
	vec2 cp2 = fract(ab.x) + fract(ab.y) > 1.0 ? vec2(ceil(ab.x), ceil(ab.y)) : vec2(floor(ab.x), floor(ab.y));

	// sampled values
	float r = rand((cp0 + cp1 + cp2) / 3.0);
	float r0 = rand(cp0);
	float r1 = rand(cp1);
	float r2 = rand(cp2);

	// ps are cps in pixel coordinates
	vec2 p0 = m * cp0;
	vec2 p1 = m * cp1;
	vec2 p2 = m * cp2;

	// blending weights
	float invMaxLen = 1. / uTileSize;
	float w0 = 1.0 - length(p0 - xy) * invMaxLen;
	float w1 = 1.0 - length(p1 - xy) * invMaxLen;
	float w2 = 1.0 - length(p2 - xy) * invMaxLen;
	float sw = w0 + w1 + w2;
	float sw2 = w0*w0 + w1*w1 + w2*w2;

	vec2 closestP = (w0 > w1 && w0 > w2) ? p0 : (w1 > w2 ? p1 : p2);
	vec2 uv0 = closestP / imageSize(image).xy;

	vec2 uv = xy / imageSize(image).xy;
	vec4 color0 = texture(uInputTexture, uv0 + randRot(cp0) * (uv - uv0) + (randUv(cp0) - 0.5) * uRandomTranslation);
	vec4 color1 = texture(uInputTexture, uv0 + randRot(cp1) * (uv - uv0) + (randUv(cp1) - 0.5) * uRandomTranslation);
	vec4 color2 = texture(uInputTexture, uv0 + randRot(cp2) * (uv - uv0) + (randUv(cp2) - 0.5) * uRandomTranslation);

	vec4 color;
	//color = vec4((w0 * r0 + w1 * r1 + w2 * r2) / sw);
	if (uBlendMode == 1) {
		color = (w0 * color0 + w1 * color1 + w2 * color2) / sw;
	} else {
		color = (w0 > w1 && w0 > w2) ? color0 : (w1 > w2 ? color1 : color2);
	}

	if (uContrastRestoration) {
		// Contrast restoration
		color = (color - vec4(0.5)) / sqrt(sw2) + vec4(0.5);
	}

	imageStore(image, co, color);
}