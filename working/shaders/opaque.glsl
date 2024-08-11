#inject
#include "common.glsl"

@edithint uColor = color
uniform Material {
    vec3 uColor;
};

INPUT(vec3, iPosition, 0);
INPUT(vec3, iNormal, 1);
INPUT(vec2, iTexCoord, 2);
VARYING(vec3, vNormal);
VARYING(vec3, vToCamera);
VARYING(vec2, vTexCoord);
VARYING(float, vDistance);
VARYING(vec4, vFragPosLightSpace);
OUTPUT(vec4, oColor, 0);

uniform mat4 uTransform;
uniform sampler2D tAlbedo;
uniform sampler2D tSpecular;

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
    vDistance = length(viewSpace.xyz);
    vFragPosLightSpace = gLightMat * worldSpace;
}
#endif

#ifdef FRAG
void main(void) {
    oColor = texture(tAlbedo, vTexCoord);
    oColor.rgb *= pow(oColor.rgb, vec3(kGamma));
    oColor.rgb *= uColor;
    float specularStrength = texture(tSpecular, vTexCoord).r;
    vec3 unitNormal = normalize(vNormal);
    vec3 unitToCamera = normalize(vToCamera);
    
    float shadow = 1.0 - _shadowCalculation(tShadowMap, vFragPosLightSpace, unitNormal, -gSunDirection);
    
    oColor.rgb *= max(gSunColor * dot(unitNormal, -gSunDirection) * shadow, 0.2);
    vec3 reflectedLight = reflect(gSunDirection, unitNormal);
    oColor.rgb += gSunColor * pow(max(dot(reflectedLight, unitToCamera) * shadow, 0.0), 10.0) * specularStrength;
    oColor.rgb = mix(oColor.rgb, gSkyColor, smoothstep(gFarPlane * 0.6, gFarPlane, vDistance));
}
#endif