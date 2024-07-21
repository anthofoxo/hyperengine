INPUT(vec2, iPosition, 0);
OUTPUT(vec4, oColor, 0);

#ifdef HE_VERT
void main(void) {
    gl_Position = vec4(iPosition * 0.5, 0.0, 1.0);
}
#endif

#ifdef HE_FRAG
void main(void) {
    oColor = vec4(1.0, 0.5, 0.0, 1.0);
}
#endif