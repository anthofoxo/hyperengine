INPUT(vec3, iPosition, 0);
INPUT(vec3, iNormal, 1);
INPUT(vec2, iTexCoord, 2);
VARYING(vec3, vNormal);
VARYING(vec2, vTexCoord);
OUTPUT(vec4, oColor, 0);
UNIFORM(mat4, uTransform);
UNIFORM(mat4, uView);
UNIFORM(mat4, uProjection);
UNIFORM(sampler2D, uSampler);

#ifdef VERT
void main(void) {
    gl_Position = uProjection * uView * uTransform * vec4(iPosition, 1.0);
    vTexCoord = iTexCoord;
    vNormal = inverse(transpose(mat3(uTransform))) * iNormal;
}
#endif

#ifdef FRAG
void main(void) {
    oColor = texture(uSampler, vTexCoord);
    oColor.rgb *= max(dot(vNormal, normalize(vec3(0, 0, 1))), 0.2);
}
#endif