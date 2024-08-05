# HyperEngine
Small game engine written in C++

## Cloning
We use submodules, clone with submodules! `--recurse-submodules`

## Building
We use [Premake5](https://premake.github.io/) as a build system. Have this installed.

### Windows
Run `premake5 vs2022` to generate a visual studio solution. This should build out of the box.

### Unix
1. Wayland and X11 are supported. Run `xdg.sh` in the `scripts` directory to generate the required headers.
1. Run `premake5 gmake2` to generate makefiles.

## Shader compilation
The shader engine defines macros to easily allow multiple shader types in one source file. Interally this will be split up correctly for the OpenGl Apis.

* `VERT` is defined if the source is being passed to the vertex shader compiler.
* `FRAG` is defined if the source is being passed to the fragment shader compiler.

Additionally some macros are defined to make defining inputs and output easier.

* `INPUT` is only expanded for vertex shaders.
* `OUTPUT` is only expanded for fragment shaders.
* `UNIFORM` is always expanded reguardless of shader types. This is purely for consistancy.
* `CONST` is always expanded reguardless of shader types. This is purely for consistancy.
* `VARYING` is defined to output from the vertex shader and input to fragment shaders. Similarly to how the `varying` specifier used to work.

See shader sources in `./working` for examples.

## Dependencies
### Directly in source tree
* [stb_image v2.30](https://github.com/nothings/stb/blob/f7f20f39fe4f206c6f19e26ebfef7b261ee59ee4/stb_image.h)
* [RenderDoc 1.6.0](https://renderdoc.org/docs/in_application_api.html)
* [debug-trap](https://github.com/nemequ/portable-snippets/blob/84abba93ff3d52c87e08ba81de1cc6615a42b72e/debug-trap/debug-trap.h)
* [ImGuiColorTextEdit 0a88824](https://github.com/BalazsJako/ImGuiColorTextEdit/tree/0a88824f7de8d0bd11d8419066caa7d3469395c4) +Local fixes
* [miniaudio v0.11.21](https://github.com/mackron/miniaudio/tree/4a5b74bef029b3592c54b6048650ee5f972c1a48)
### From submodules
* [GLFW 3.4](https://github.com/glfw/glfw/tree/3.4)
* [Glad 3.3+](https://gen.glad.sh/#generator=c&api=gl%3D3.3&profile=gl%3Dcore%2Cgles1%3Dcommon&extensions=GL_ARB_direct_state_access%2CGL_ARB_texture_filter_anisotropic%2CGL_ARB_texture_storage%2CGL_EXT_texture_filter_anisotropic%2CGL_KHR_debug)
* [glm 1.0.1](https://github.com/g-truc/glm/tree/1.0.1)
* [ImGui v1.90.9-docking](https://github.com/ocornut/imgui/tree/v1.90.9-docking)
* [assimp v5.0.1](https://github.com/assimp/assimp/tree/v5.0.1)
* [EnTT v3.13.2](https://github.com/skypjack/entt/tree/v3.13.2)
* [Lua 5.4.6](https://github.com/anthofoxo/lua/tree/5.4.6)
* [ImGuizmo ba662b1](https://github.com/CedricGuillemet/ImGuizmo/tree/ba662b119d64f9ab700bb2cd7b2781f9044f5565)