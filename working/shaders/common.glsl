// --- Shader documentation ---
// All shader files should begin with `#inject`,
// This will cause the HyperEngine shader engine to include the `#version` directive and proper `#define`s.
//
// You can use `#include` in your shaders. This will include files from the `./shaders` directory.
// You likely want to `#include "common"` at the top of all your shaders.
//
// --- Engine pragmas ----
// HyperEngine can read back values from the shader using `@` pragmas.
// Operates similarly to `#pragma` but with unique syntax for parsing.
//
// 0 = false ; 1 = true
//
// Enable or disable backface culling, Enabled by default
// @property cull = 0|1
// 
// Give the editor hints on how to render the uniform
// @edithint <uniform> = hidden|color
//
// "hidden" will completely remove the control from the property panel.
// This is generally used for uniforms the engine runtime fills in.
//
// "color" will present a color gui instead of float sliders
//
//
// You can `#define FAST` after the `#inject` to trigger your shader to run lower quality.
// This currently affects shadow sampling and terrain texture mapping
//
// Example:
// #inject
// #define FAST
// #include "common"
// ...

#ifdef VERT
#   define INPUT(type, name, index) layout(location = index) in type name
#   define OUTPUT(type, name, index)
#   define VARYING(type, name) out type name
#endif
#ifdef FRAG
#   define INPUT(type, name, index)
#   define OUTPUT(type, name, index) layout(location = index) out type name
#   define VARYING(type, name) in type name
#endif

layout(std140) uniform EngineData {
    mat4 gProjection;
    mat4 gView;
    mat4 gLightMat;
    vec3 gSkyColor;
    float gFarPlane;
    vec3 gSunDirection;
    float gTime;
    vec3 gSunColor;
};

float saturate(float value) {
    return clamp(value, 0.0, 1.0);
}

vec3 saturate(vec3 value) {
    return clamp(value, 0.0, 1.0);
}

float map(float value, float min1, float max1, float min2, float max2) {
    return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

vec3 nonUniformScale(mat4 matrix, vec3 normal) {
    return transpose(inverse(mat3(matrix))) * normal;
}

vec4 hash4(vec2 p) {
    return fract(sin(vec4(1.0 + dot(p, vec2(37.0, 17.0)),
                          2.0 + dot(p, vec2(11.0, 47.0)),
                          3.0 + dot(p, vec2(41.0, 29.0)),
                          4.0 + dot(p, vec2(23.0, 31.0)))) * 103.0);
}

#ifdef FRAG
// See: https://iquilezles.org/articles/texturerepetition/
vec4 textureNoTile(sampler2D samp, in vec2 uv) {
    ivec2 iuv = ivec2(floor(uv));
    vec2 fuv = fract(uv);

    // generate per-tile transform
    vec4 ofa = hash4(iuv + ivec2(0, 0));
    vec4 ofb = hash4(iuv + ivec2(1, 0));
    vec4 ofc = hash4(iuv + ivec2(0, 1));
    vec4 ofd = hash4(iuv + ivec2(1, 1));
    
    vec2 ddx = dFdx(uv); // Fragment shader only
    vec2 ddy = dFdy(uv); // Fragment shader only

    // transform per-tile uvs
    ofa.zw = sign(ofa.zw - 0.5);
    ofb.zw = sign(ofb.zw - 0.5);
    ofc.zw = sign(ofc.zw - 0.5);
    ofd.zw = sign(ofd.zw - 0.5);
    
    // uv's, and derivatives (for correct mipmapping)
    vec2 uva = uv * ofa.zw + ofa.xy, ddxa = ddx * ofa.zw, ddya = ddy * ofa.zw;
    vec2 uvb = uv * ofb.zw + ofb.xy, ddxb = ddx * ofb.zw, ddyb = ddy * ofb.zw;
    vec2 uvc = uv * ofc.zw + ofc.xy, ddxc = ddx * ofc.zw, ddyc = ddy * ofc.zw;
    vec2 uvd = uv * ofd.zw + ofd.xy, ddxd = ddx * ofd.zw, ddyd = ddy * ofd.zw;
    
    // fetch and blend
    vec2 b = smoothstep(0.25, 0.75, fuv);
    
    return mix(mix(textureGrad(samp, uva, ddxa, ddya),
                   textureGrad(samp, uvb, ddxb, ddyb), b.x),
               mix(textureGrad(samp, uvc, ddxc, ddyc),
                   textureGrad(samp, uvd, ddxd, ddyd), b.x), b.y);
}
#endif

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

#define NUM_POISSON_TAPS 64

const vec2 kPoissonTaps[NUM_POISSON_TAPS] = vec2[](
    vec2(-0.884081, 0.124488),
    vec2(-0.714377, 0.027940),
    vec2(-0.747945, 0.227922),
    vec2(-0.939609, 0.243634),
    vec2(-0.985465, 0.045534),
    vec2(-0.861367, -0.136222),
    vec2(-0.881934, 0.396908),
    vec2(-0.466938, 0.014526),
    vec2(-0.558207, 0.212662),
    vec2(-0.578447, -0.095822),
    vec2(-0.740266, -0.095631),
    vec2(-0.751681, 0.472604),
    vec2(-0.553147, -0.243177),
    vec2(-0.674762, -0.330730),
    vec2(-0.402765, -0.122087),
    vec2(-0.319776, -0.312166),
    vec2(-0.413923, -0.439757),
    vec2(-0.979153, -0.201245),
    vec2(-0.865579, -0.288695),
    vec2(-0.243704, -0.186378),
    vec2(-0.294920, -0.055748),
    vec2(-0.604452, -0.544251),
    vec2(-0.418056, -0.587679),
    vec2(-0.549156, -0.415877),
    vec2(-0.238080, -0.611761),
    vec2(-0.267004, -0.459702),
    vec2(-0.100006, -0.229116),
    vec2(-0.101928, -0.380382),
    vec2(-0.681467, -0.700773),
    vec2(-0.763488, -0.543386),
    vec2(-0.549030, -0.750749),
    vec2(-0.809045, -0.408738),
    vec2(-0.388134, -0.773448),
    vec2(-0.429392, -0.894892),
    vec2(-0.131597, 0.065058),
    vec2(-0.275002, 0.102922),
    vec2(-0.106117, -0.068327),
    vec2(-0.294586, -0.891515),
    vec2(-0.629418, 0.379387),
    vec2(-0.407257, 0.339748),
    vec2(0.071650, -0.384284),
    vec2(0.022018, -0.263793),
    vec2(0.003879, -0.136073),
    vec2(-0.137533, -0.767844),
    vec2(-0.050874, -0.906068),
    vec2(0.114133, -0.070053),
    vec2(0.163314, -0.217231),
    vec2(-0.100262, -0.587992),
    vec2(-0.004942, 0.125368),
    vec2(0.035302, -0.619310),
    vec2(0.195646, -0.459022),
    vec2(0.303969, -0.346362),
    vec2(-0.678118, 0.685099),
    vec2(-0.628418, 0.507978),
    vec2(-0.508473, 0.458753),
    vec2(0.032134, -0.782030),
    vec2(0.122595, 0.280353),
    vec2(-0.043643, 0.312119),
    vec2(0.132993, 0.085170),
    vec2(-0.192106, 0.285848),
    vec2(0.183621, -0.713242),
    vec2(0.265220, -0.596716),
    vec2(-0.009628, -0.483058),
    vec2(-0.018516, 0.435703)
);

vec2 samplePoisson(int index) {
    return kPoissonTaps[index % NUM_POISSON_TAPS];
}

#undef NUM_POISSON_TAPS

#ifdef FRAG

float _shadowCalculation(sampler2D samp, vec4 fragPosLightSpace, vec3 normal, vec3 sunDirection) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
     if(projCoords.z > 1.0)
        return 0.0;
    
    float currentDepth = projCoords.z;
    float bias = max(0.001 * (1.0 - dot(normal, -sunDirection)), 0.0);

#ifdef FAST
    float pcfDepth = texture(samp, projCoords.xy).r;
    return currentDepth - bias > pcfDepth ? 1.0 : 0.0;
#else
    vec2 texelSize = 1.0 / textureSize(samp, 0);
    float shadow = 0.0;

    int offset = 3;
    int sampleCount = offset * offset;

    for(int i = 0; i < sampleCount; ++i) {
        // vec2 offset = vec2(random((gl_FragCoord.xy) + gTime * 0.0199814), random(gl_FragCoord.yx - gTime * 0.074115)) * 2.0 - 1.0;
        vec2 offset = vec2(i / offset, i % offset) - (float(offset) / 2.0);
        offset += samplePoisson(i);

        float pcfDepth = texture(samp, projCoords.xy + offset * texelSize).r;
        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
    }
    
    return shadow / float(sampleCount);
#endif
}

#endif