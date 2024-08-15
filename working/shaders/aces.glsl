#inject
#include "common.glsl"

// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl

VARYING(vec2, vUv);
OUTPUT(vec4, oColor, 0);
uniform sampler2D uAux;
const float kGamma = 2.2;

#ifdef VERT
const vec2 kPositions[4] = vec2[](
	vec2(-1.0, 1.0),
	vec2(-1.0, -1.0),
	vec2(1.0, 1.0),
	vec2(1.0, -1.0)
);

void main(void) {
	gl_Position = vec4(kPositions[gl_VertexID], 0.0, 1.0);
	vUv = kPositions[gl_VertexID] * 0.5 + 0.5;
}
#endif

#ifdef FRAG

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
const mat3 kACESInput = mat3
(
	vec3(0.59719, 0.07600, 0.02840),
	vec3(0.35458, 0.90834, 0.13383),
	vec3(0.04823, 0.01566, 0.83777)
);

// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 kACESOutput = mat3
(
	vec3( 1.60475, -0.10208, -0.00327),
	vec3(-0.53108,  1.10813, -0.07276),
	vec3(-0.07367, -0.00605,  1.07602)
);

vec3 RRTAndODTFit(vec3 v) {
	vec3 a = v * (v + 0.0245786) - 0.000090537;
	vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
	return a / b;
}

vec3 ACESFitted(vec3 color) {
	return saturate(kACESOutput * RRTAndODTFit(kACESInput * color));
}

void main(void) {
	oColor = vec4(texture(uAux, vUv).rgb, 1.0);
	oColor.rgb = ACESFitted(oColor.rgb);
	oColor.rgb = pow(oColor.rgb, vec3(1.0 / kGamma));
	oColor.rgb = saturate(oColor.rgb);
}
#endif