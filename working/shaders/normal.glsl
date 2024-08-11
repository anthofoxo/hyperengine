#inject
#include "common.glsl"

INPUT(vec3, iPosition, 0);
INPUT(vec3, iNormal, 1);
INPUT(vec2, iTexCoord, 2);
INPUT(vec3, iTangent, 3);
VARYING(vec3, vToCamera);
VARYING(vec3, vNormal);
VARYING(vec2, vTexCoord);
VARYING(float, vDistance);
VARYING(mat3, vTbn);
VARYING(vec4, vFragPosLightSpace);
OUTPUT(vec4, oColor, 0);

uniform mat4 uTransform;
uniform sampler2D tAlbedo;
uniform sampler2D tSpecular;
uniform sampler2D tNormal;

@edithint tShadowMap = hidden
uniform sampler2D tShadowMap;

const float kGamma = 2.2;

#ifdef VERT
void main(void) {
    vec4 worldSpace = uTransform * vec4(iPosition, 1.0);
    vec4 viewSpace = gView * worldSpace;
    gl_Position = gProjection * viewSpace;
    vTexCoord = iTexCoord;
    vToCamera = (inverse(gView) * vec4(0.0, 0.0, 0.0, 1.0)).xyz - worldSpace.xyz;
    vDistance = length(viewSpace.xyz);

    vec3 T = normalize(vec3(uTransform * vec4(iTangent, 0.0)));
    vec3 N = normalize(vec3(uTransform * vec4(iNormal, 0.0)));
    T = normalize(T - dot(T, N) * N);
    vTbn = mat3(T, cross(N, T), N);
    
    vNormal = N;
    
    vFragPosLightSpace = gLightMat * worldSpace;
}
#endif

#ifdef FRAG
void main(void) {
    oColor = texture(tAlbedo, vTexCoord);
    oColor.rgb *= pow(oColor.rgb, vec3(kGamma));
    float specularStrength = texture(tSpecular, vTexCoord).r;

    vec3 unitNormal = texture(tNormal, vTexCoord).xyz * 2.0 - 1.0;
    unitNormal = normalize(vTbn * normalize(unitNormal));

    float shadow = 1.0 - _shadowCalculation(tShadowMap, vFragPosLightSpace, unitNormal, -gSunDirection);

    vec3 unitToCamera = normalize(vToCamera);
    oColor.rgb *= max(gSunColor * dot(unitNormal, -gSunDirection) * shadow, 0.2);
    vec3 reflectedLight = reflect(gSunDirection, unitNormal);
    oColor.rgb += gSunColor * pow(max(dot(reflectedLight, unitToCamera) * shadow, 0.0), 10.0) * specularStrength;
    oColor.rgb = mix(oColor.rgb, gSkyColor, smoothstep(gFarPlane * 0.6, gFarPlane, vDistance));
}
#endif