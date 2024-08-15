#inject
#include "common.glsl"

@property cull = 0

INPUT(vec3, iPosition, 0);
INPUT(vec2, iTexCoord, 2);
VARYING(vec2, vTexCoord);

uniform mat4 uTransform;
uniform sampler2D tAlbedo;

#ifdef VERT
void main(void) {
	gl_Position = gLightMat * uTransform * vec4(iPosition, 1.0);
	vTexCoord = iTexCoord;
}
#endif

#ifdef FRAG
void main(void) {
	vec4 color = texture(tAlbedo, vTexCoord);
	if (color.a < 0.5) discard;
}
#endif