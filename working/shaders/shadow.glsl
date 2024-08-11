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

INPUT(vec3, iPosition, 0);
INPUT(vec3, iNormal, 1);
INPUT(vec2, iTexCoord, 2);
VARYING(vec2, vTexCoord);

uniform mat4 uTransform;
uniform sampler2D tAlbedo;

#ifdef VERT
void main(void) {
    vec4 worldSpace = uTransform * vec4(iPosition, 1.0);
    vec4 viewSpace = gView * worldSpace;
    gl_Position = gProjection * viewSpace;
    vTexCoord = iTexCoord;
}
#endif

#ifdef FRAG
void main(void) {
    vec4 color = texture(tAlbedo, vTexCoord);
    if (color.a < 0.5) discard;
}
#endif