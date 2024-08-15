# HyperEngine
HyperEngine is an early-sage interactive application and rendering engine for Windows and Linux.

## Getting Started
Visual Studio 2019 or 2022 is recommended.

<ins>**1. Downloading the repo:**</ins>

Start by cloning the repo with `git clone --recurse-submodules https://github.com/anthofoxo/hyperengine`.

If the repository was cloned non-recursively previously, use `git submodule update --init` to clone the necessary submodules.

<ins>**2. Configuring the dependencies: (Linux only)**</ins>

HyperEngine supports wayland which required generating headers. Run `./scripts/xdg.sh` to generate the `xdg` headers.

<ins>**3. Building:**</ins>

We use [Premake5](https://premake.github.io/) as a build system. Have this installed. Once installed use `premake5 vs2019` or `premake5 vs2022` to generate Visual Studio project files or `premake5 gmake2` to generate makefiles. When using make use the `config` option to select the build configuration. Eg: `make config=debug`.

## Debugging Features

### RenderDoc
HyperEngine has builtin [RenderDoc](https://renderdoc.org/) support. If RenderDoc can be found on your system then HyperEngine will automatically attach RenderDoc.

The default overlays are disable but will still be functional. F12 will still capture frames by default. You can attach it to HyperEngine at any time via RenderDoc with the `File>Attach to Running Instance`.

### Tracy
[Tracy](https://github.com/wolfpld/tracy/releases/tag/v0.11.0) is a frame profiler that is default supported. You can attach the tracy profiler at anytime.

### TODO
We are working on file and directory watchers and dynamic asset reloading but is not complete.

## Engine Defined Shader Macros
HyperEngine will predefine some macros to make shaders easier to write.

A macro will be defined for each shader compilation stage. The engine internally splits this up correctly for the OpenGL Api.

* `VERT` is defined if the source is being passed to the vertex shader compiler.
* `FRAG` is defined if the source is being passed to the fragment shader compiler.

Some helper macros are defined help keep variables in one section and to reduce errors.

* `INPUT` is only expanded for vertex shaders.
* `OUTPUT` is only expanded for fragment shaders.
* `VARYING` is defined to output from the vertex shader and input to fragment shaders. Similarly to how the `varying` specifier used to work.

See shader sources in `./working` for examples.

## Dependencies
HyperEngine has a few dependencies listed below. If you cloned with submodules then you already have them all.

### In source tree
* [stb_image v2.30](https://github.com/nothings/stb/blob/f7f20f39fe4f206c6f19e26ebfef7b261ee59ee4/stb_image.h)
* [RenderDoc 1.6.0](https://renderdoc.org/docs/in_application_api.html)
* [debug-trap](https://github.com/nemequ/portable-snippets/blob/84abba93ff3d52c87e08ba81de1cc6615a42b72e/debug-trap/debug-trap.h)
* [ImGuiColorTextEdit 00c9fcb](https://github.com/santaclose/ImGuiColorTextEdit/tree/00c9fcb39e5dc82a4eefcf2e97f29e5e74381895) +Local fixes
* [miniaudio v0.11.21](https://github.com/mackron/miniaudio/tree/4a5b74bef029b3592c54b6048650ee5f972c1a48)
* [stb_vorbis v1.22](https://github.com/nothings/stb/blob/f75e8d1cad7d90d72ef7a4661f1b994ef78b4e31/stb_vorbis.c)
* [tinyfiledialogs v3.18.2](https://sourceforge.net/p/tinyfiledialogs/code/ci/29c1b354d75825209adf8cc1979c425885a64d32/tree/)
### Submodules
* [GLFW 3.4](https://github.com/glfw/glfw/tree/3.4)
* [Glad 3.3+](https://gen.glad.sh/#generator=c&api=gl%3D3.3&profile=gl%3Dcore%2Cgles1%3Dcommon&extensions=GL_ARB_direct_state_access%2CGL_ARB_texture_filter_anisotropic%2CGL_ARB_texture_storage%2CGL_EXT_texture_filter_anisotropic%2CGL_KHR_debug)
* [glm 1.0.1](https://github.com/g-truc/glm/tree/1.0.1)
* [ImGui v1.90.9-docking](https://github.com/ocornut/imgui/tree/v1.90.9-docking)
* [assimp v5.0.1](https://github.com/assimp/assimp/tree/v5.0.1)
* [EnTT v3.13.2](https://github.com/skypjack/entt/tree/v3.13.2)
* [Lua 5.4.6](https://github.com/anthofoxo/lua/tree/5.4.6)
* [ImGuizmo ba662b1](https://github.com/CedricGuillemet/ImGuizmo/tree/ba662b119d64f9ab700bb2cd7b2781f9044f5565)
* [Tracy v0.11.0](https://github.com/wolfpld/tracy/tree/v0.11.0)
* [spdlog v1.14.1](https://github.com/gabime/spdlog/tree/v1.14.1)