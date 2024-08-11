@property cull = 0

layout(std140) uniform EngineData {
    mat4 gProjection;
    mat4 gView;
    mat4 gLightMat;
    vec3 gSkyColor;
    float gFarPlane;
    vec3 gSunDirection;
    vec3 gSunColor;
};

uniform Material {
    float uCutoff;
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

uniform sampler2D tShadowMap;

const float kGamma = 2.2;

#ifdef VERT
void main(void) {
    vec4 worldSpace = uTransform * vec4(iPosition, 1.0);
    vec4 viewSpace = gView * worldSpace;
    gl_Position = gProjection * viewSpace;
    vTexCoord = iTexCoord;
    vNormal = transpose(inverse(mat3(uTransform))) * iNormal;
    vToCamera = (inverse(gView) * vec4(0.0, 0.0, 0.0, 1.0)).xyz - worldSpace.xyz;
    vDistance = length(viewSpace);
    vFragPosLightSpace = gLightMat * worldSpace;
}
#endif

#ifdef FRAG
float ShadowCalculation(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
     if(projCoords.z > 1.0)
        return 0.0;
    
    vec2 texelSize = 1.0 / textureSize(tShadowMap, 0);
   
    
    float currentDepth = projCoords.z;
    float bias = max(0.003 * (1.0 - dot(vNormal, gSunDirection)), 0.0);
    
    float shadow = 0.0;
   
    
    for(int y = -1; y <= 1; ++y) {
        for(int x = -1; x <= 1; ++x) {
            float pcfDepth = texture(tShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
        }
    }
    
    return shadow / 9.0;
}


void main(void) {
    oColor = texture(tAlbedo, vTexCoord);
    oColor.rgb *= pow(oColor.rgb, vec3(kGamma));
    if (oColor.a < 0.5) discard;
    oColor.a = 1.0;
    
    float shadow = 1.0 - ShadowCalculation(vFragPosLightSpace);
    
    vec3 unitNormal = normalize(vNormal);
    vec3 unitToCamera = normalize(vToCamera);
    if (!gl_FrontFacing) unitNormal *= -1.0;
    oColor.rgb *= max(gSunColor * dot(unitNormal, -gSunDirection) * shadow, 0.2);
    oColor.rgb = mix(oColor.rgb, gSkyColor, smoothstep(gFarPlane * 0.6, gFarPlane, vDistance));
}
#endif