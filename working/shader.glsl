INPUT(vec2, iPosition, 0);
VARYING(vec2, vTexCoord);
OUTPUT(vec4, oColor, 0);
UNIFORM(mat4, uTransform);
UNIFORM(mat4, uView);
UNIFORM(mat4, uProjection);
UNIFORM(sampler2D, uSampler);

#ifdef VERT
void main(void) {
    gl_Position = uProjection * uView * uTransform * vec4(iPosition, 0.0, 1.0);
    vTexCoord = iPosition * 0.5 + 0.5;
}
#endif

#ifdef FRAG
void main(void) {
    oColor = texture(uSampler, vTexCoord);
}
#endif