//////////////////////////////////////////////////////
// G-Buffer related functions
//
// This version si newer than gbuffer.inc.glsl and intended to replace the latter

struct GFragment {
	vec3 baseColor;
	vec3 normal;
	vec3 ws_coord;
	uint material_id;
	float roughness;
	float metallic;
	vec3 emission;
	float alpha;

	// Used for G-impostors but not in gbuffer (converted to regular normal)
	vec4 lean1;
	vec4 lean2;
};

/**
 * Apply mix to all components of a GFragment
 */
GFragment LerpGFragment(GFragment ga, GFragment gb, float t) {
	GFragment g;
	g.baseColor = mix(ga.baseColor, gb.baseColor, t);
	g.ws_coord = mix(ga.ws_coord, gb.ws_coord, t);
	g.metallic = mix(ga.metallic, gb.metallic, t);
	g.roughness = mix(ga.roughness, gb.roughness, t);
	g.emission = mix(ga.emission, gb.emission, t);
	g.alpha = mix(ga.alpha, gb.alpha, t);
	g.lean1 = mix(ga.lean1, gb.lean1, t);
	g.lean2 = mix(ga.lean2, gb.lean2, t);
	g.material_id = ga.material_id; // cannot be interpolated

	// Normal interpolation
	float wa = (1 - t) * ga.alpha;
	float wb = t * gb.alpha;

	// Avoid trouble with NaNs in normals
	if (wa == 0) ga.normal = vec3(0.0);
	if (wb == 0) gb.normal = vec3(0.0);

	g.normal = normalize(wa * ga.normal + wb * gb.normal);

	// Naive interpolation
	//g.normal = normalize(mix(ga.normal, gb.normal, t));
	return g;
}

void unpackGFragment(
	in sampler2D gbuffer1,
	in usampler2D gbuffer2,
	in usampler2D gbuffer3,
	in ivec2 coords, 
	out GFragment fragment)
{
	vec4 data1 = texelFetch(gbuffer1, coords, 0);
	uvec4 data2 = texelFetch(gbuffer2, coords, 0);
	uvec4 data3 = texelFetch(gbuffer3, coords, 0);
	vec2 tmp;

	tmp = unpackHalf2x16(data2.y);
	fragment.baseColor = vec3(unpackHalf2x16(data2.x), tmp.x);
	fragment.normal = normalize(vec3(tmp.y, unpackHalf2x16(data2.z)));
	fragment.ws_coord = data1.xyz;
	fragment.material_id = data2.w;
	fragment.roughness = data1.w;

	tmp = unpackHalf2x16(data3.y);
	fragment.emission = vec3(unpackHalf2x16(data3.x), tmp.x);
	fragment.metallic = tmp.y;
}

void packGFragment(
	in GFragment fragment,
	out vec4 gbuffer_color1,
	out uvec4 gbuffer_color2,
	out uvec4 gbuffer_color3)
{
	gbuffer_color1 = vec4(fragment.ws_coord, fragment.roughness);
	gbuffer_color2 = uvec4(
		packHalf2x16(fragment.baseColor.xy),
		packHalf2x16(vec2(fragment.baseColor.z, fragment.normal.x)),
		packHalf2x16(fragment.normal.yz),
		fragment.material_id
	);
	gbuffer_color3 = uvec4(
		packHalf2x16(fragment.emission.xy),
		packHalf2x16(vec2(fragment.emission.z, fragment.metallic)),
		packHalf2x16(vec2(0.0, 0.0)),
		0
	);
}


//////////////////////////////////////////////////////
// Reserved material IDs

const uint noMaterial = 0;
const uint skyboxMaterial = 1;
const uint forwardAlbedoMaterial = 2;
const uint forwardBaseColorMaterial = 3;
const uint forwardNormalMaterial = 4;
const uint pbrMaterial = 5;
const uint pbrMetallicRoughnessMaterial = 6;
const uint iridescentMaterial = 7;
const uint worldMaterial = 8;
const uint iblMaterial = 9;
const uint farCloudTestMaterial = 10;
