layout(std140) uniform EngineData {
    mat4 gProjection;
    mat4 gView;
    vec3 gSkyColor;
    float gFarPlane;
};

uniform Material {
    float uTiling;
};

INPUT(vec3, iPosition, 0);
INPUT(vec3, iNormal, 1);
INPUT(vec2, iTexCoord, 2);
VARYING(vec3, vNormal);
VARYING(vec3, vToCamera);
VARYING(vec2, vTexCoord);
VARYING(float, vDistance);
OUTPUT(vec4, oColor, 0);

uniform mat4 uTransform;
uniform sampler2D tAlbedo;
uniform sampler2D tSpecular;
const vec3 kLightDirection = normalize(vec3(0, -3, -1));
const vec3 kLightColor = vec3(1.0, 1.0, 1.0);

#ifdef VERT
void main(void) {
    vec4 worldSpace = uTransform * vec4(iPosition, 1.0);
    vec4 viewSpace = gView * worldSpace;
    gl_Position = gProjection * viewSpace;
    vTexCoord = iTexCoord * uTiling;
    vNormal = transpose(inverse(mat3(uTransform))) * iNormal;
    vToCamera = (inverse(gView) * vec4(0.0, 0.0, 0.0, 1.0)).xyz - worldSpace.xyz;
    vDistance = length(viewSpace.xyz);
}
#endif

#ifdef FRAG
void main(void) {
    oColor = texture(tAlbedo, vTexCoord);
    float specularStrength = texture(tSpecular, vTexCoord).r;
    vec3 unitNormal = normalize(vNormal);
    vec3 unitToCamera = normalize(vToCamera);
    oColor.rgb *= max(kLightColor * dot(unitNormal, -kLightDirection), 0.2);
    vec3 reflectedLight = reflect(kLightDirection, unitNormal);
    oColor.rgb += kLightColor * pow(max(dot(reflectedLight, unitToCamera), 0.0), 10.0) * specularStrength;
    oColor.rgb = mix(oColor.rgb, gSkyColor, smoothstep(gFarPlane * 0.6, gFarPlane, vDistance));
}
#endif