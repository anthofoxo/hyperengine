#inject
#include "common.glsl"
#extension GL_ARB_gpu_shader5 : enable

@stage geom = 1
@property cull = 0

uniform mat4 uTransform;
uniform sampler2D tAlbedo;

#ifdef VERT
layout(location = 0) in vec3 iPosition;
layout(location = 2) in vec2 iTexCoord;
out vec2 vgTexCoord;

void main(void) {
    gl_Position = uTransform * vec4(iPosition, 1.0);
    vgTexCoord = iTexCoord;
}
#endif

#ifdef GEOM
layout(triangles, invocations = 4) in;
layout(triangle_strip, max_vertices = 3) out;

in vec2 vgTexCoord[];
out vec2 vfTexCoord;

void main(void) {
    for (int i = 0; i < 3; ++i) {
        // gl_Position = lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
        gl_Position = gLightMat[gl_InvocationID] * gl_in[i].gl_Position;
        vfTexCoord = vgTexCoord[i];
        gl_Layer = gl_InvocationID;
        EmitVertex();
    }
    EndPrimitive();
}
#endif

#ifdef FRAG
in vec2 vfTexCoord;

void main(void) {
    vec4 color = texture(tAlbedo, vfTexCoord);
    if (color.a < 0.5) discard;
}
#endif