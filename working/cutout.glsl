@property cull = 0

layout(std140) uniform EngineData {
    mat4 gProjection;
    mat4 gView;
    vec3 gSkyColor;
    float gFarPlane;
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
OUTPUT(vec4, oColor, 0);

uniform mat4 uTransform;
uniform sampler2D tAlbedo;
const vec3 kLightDirection = normalize(vec3(0, -3, -1));
const vec3 kLightColor = vec3(1.0, 1.0, 1.0);

#ifdef VERT
void main(void) {
    vec4 worldSpace = uTransform * vec4(iPosition, 1.0);
    vec4 viewSpace = gView * worldSpace;
    gl_Position = gProjection * viewSpace;
    vTexCoord = iTexCoord;
    vNormal = transpose(inverse(mat3(uTransform))) * iNormal;
    vToCamera = (inverse(gView) * vec4(0.0, 0.0, 0.0, 1.0)).xyz - worldSpace.xyz;
    vDistance = length(viewSpace);
}
#endif

#ifdef FRAG
void main(void) {
    oColor = texture(tAlbedo, vTexCoord);
    if (oColor.a < uCutoff) discard;
    oColor.a = 1.0;
    vec3 unitNormal = normalize(vNormal);
    vec3 unitToCamera = normalize(vToCamera);
    if (!gl_FrontFacing) unitNormal *= -1.0;
    oColor.rgb *= max(kLightColor * dot(unitNormal, -kLightDirection), 0.2);
    oColor.rgb = mix(oColor.rgb, gSkyColor, smoothstep(gFarPlane * 0.6, gFarPlane, vDistance));
}
#endif