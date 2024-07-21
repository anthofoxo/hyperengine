INPUT(vec2, iPosition, 0);
OUTPUT(vec4, oColor, 0);
UNIFORM(mat4, uTransform);
UNIFORM(mat4, uView);
UNIFORM(mat4, uProjection);

#ifdef HE_VERT
void main(void) {
    gl_Position = uProjection * uView * uTransform * vec4(iPosition, 0.0, 1.0);
}
#endif

#ifdef HE_FRAG
void main(void) {
    oColor = vec4(1.0, 0.5, 0.0, 1.0);
}
#endif