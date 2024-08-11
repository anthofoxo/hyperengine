layout(std140) uniform EngineData {
    mat4 gProjection;
    mat4 gView;
    mat4 gLightMat;
    vec3 gSkyColor;
    float gFarPlane;
    vec3 gSunDirection;
    vec3 gSunColor;
};

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
    float specularStrength = texture(tSpecular, vTexCoord).r;

    vec3 unitNormal = texture(tNormal, vTexCoord).xyz * 2.0 - 1.0;
    unitNormal = normalize(vTbn * normalize(unitNormal));

    float shadow = 1.0 - ShadowCalculation(vFragPosLightSpace);

    vec3 unitToCamera = normalize(vToCamera);
    oColor.rgb *= max(gSunColor * dot(unitNormal, -gSunDirection) * shadow, 0.2);
    vec3 reflectedLight = reflect(gSunDirection, unitNormal);
    oColor.rgb += gSunColor * pow(max(dot(reflectedLight, unitToCamera) * shadow, 0.0), 10.0) * specularStrength;
    oColor.rgb = mix(oColor.rgb, gSkyColor, smoothstep(gFarPlane * 0.6, gFarPlane, vDistance));
}
#endif