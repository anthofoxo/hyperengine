#inject
#include "common.glsl"

// https://habr.com/en/articles/442924/

uniform Material {
    vec4 uTiling;
};

INPUT(vec3, iPosition, 0);
INPUT(vec3, iNormal, 1);
INPUT(vec2, iTexCoord, 2);
VARYING(vec3, vNormal);
VARYING(vec3, vToCamera);
VARYING(vec2, vTexCoord);
VARYING(vec3, vPosition);
VARYING(vec4, vFragPosLightSpace);
OUTPUT(vec4, oColor, 0);

uniform mat4 uTransform;
uniform sampler2D tAlbedo0;
uniform sampler2D tAlbedo1;
uniform sampler2D tAlbedo2;
uniform sampler2D tAlbedo3;
uniform sampler2D tBlendmap;

@edithint tShadowMap = hidden
uniform sampler2D tShadowMap;

const float kGamma = 2.2;

#ifdef VERT
void main(void) {
    vec4 worldSpace = uTransform * vec4(iPosition, 1.0);
    vec4 viewSpace = gView * worldSpace;
    gl_Position = gProjection * viewSpace;
    vTexCoord = iTexCoord;
    vNormal = nonUniformScale(uTransform, iNormal);
    vToCamera = (inverse(gView) * vec4(0.0, 0.0, 0.0, 1.0)).xyz - worldSpace.xyz;
    vPosition = viewSpace.xyz;
    vFragPosLightSpace = gLightMat * worldSpace;
}
#endif

#ifdef FRAG
void main(void) {
    vec3 blendmapColor = texture(tBlendmap, vTexCoord).rgb;
    vec4 bias = vec4(1.0 - (blendmapColor.r + blendmapColor.g + blendmapColor.b), blendmapColor);
    vec4 color0 = pow(textureNoTile(tAlbedo0, vTexCoord * uTiling.x), vec4(kGamma));
    vec4 color1 = pow(textureNoTile(tAlbedo1, vTexCoord * uTiling.y), vec4(kGamma));
    vec4 color2 = pow(textureNoTile(tAlbedo2, vTexCoord * uTiling.z), vec4(kGamma));
    vec4 color3 = pow(textureNoTile(tAlbedo3, vTexCoord * uTiling.w), vec4(kGamma));

#ifdef USE_OLD_BLENDING
    oColor = vec4(color0.rgb * bias.x + color1.rgb * bias.y + color2.rgb * bias.z + color3.rgb * bias.w, 1.0);
#else
    color0.a = dot(color0.rgb, vec3(0.299, 0.587, 0.114));
    color1.a = dot(color1.rgb, vec3(0.299, 0.587, 0.114));
    color2.a = dot(color2.rgb, vec3(0.299, 0.587, 0.114));
    color3.a = dot(color3.rgb, vec3(0.299, 0.587, 0.114));

    float depth = 0.2;
    float ma = max(max(color0.a + bias.x, color1.a + bias.y), max(color2.a + bias.z, color3.a + bias.w)) - depth;
    float b0 = max(color0.a + bias.x - ma, 0.0);
    float b1 = max(color1.a + bias.y - ma, 0.0);
    float b2 = max(color2.a + bias.z - ma, 0.0);
    float b3 = max(color3.a + bias.w - ma, 0.0);
    oColor = vec4((color0.rgb * b0 + color1.rgb * b1 + color2.rgb * b2 + color3.rgb * b3)/(b0+b1+b2+b3), 1.0);
#endif

    vec3 unitNormal = normalize(vNormal);
    vec3 unitToCamera = normalize(vToCamera);
    
    float dist = length(vPosition);

    
    float shadow =  _shadowCalculation(tShadowMap, vFragPosLightSpace, unitNormal, -gSunDirection);
    float transStart = 50.0 * 0.9;
    float transLen = 50.0 - transStart;
    shadow *= 1.0 - saturate((dist - transStart) / transLen);
    shadow = 1.0 - shadow;
    
    
    oColor.rgb *= max(gSunColor * dot(unitNormal, -gSunDirection) * shadow, 0.2);
    oColor.rgb = mix(oColor.rgb, gSkyColor, smoothstep(gFarPlane * 0.6, gFarPlane, length(vPosition)));
}
#endif