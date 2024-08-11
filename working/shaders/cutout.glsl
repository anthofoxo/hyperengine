#inject
#include "common.glsl"
@property cull = 0

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
    vDistance = length(viewSpace);
    vFragPosLightSpace = gLightMat * worldSpace;
}
#endif

#ifdef FRAG
void main(void) {
    oColor = texture(tAlbedo, vTexCoord);
    oColor.rgb *= pow(oColor.rgb, vec3(kGamma));
    if (oColor.a < 0.5) discard;
    oColor.a = 1.0;
     vec3 unitNormal = normalize(vNormal);
    if (!gl_FrontFacing) unitNormal *= -1.0;

    float shadow = 1.0 - _shadowCalculation(tShadowMap, vFragPosLightSpace, unitNormal, -gSunDirection);
    vec3 unitToCamera = normalize(vToCamera);
    
    oColor.rgb *= max(gSunColor * dot(unitNormal, -gSunDirection) * shadow, 0.2);
    oColor.rgb = mix(oColor.rgb, gSkyColor, smoothstep(gFarPlane * 0.6, gFarPlane, vDistance));
}
#endif