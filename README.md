# HyperEngine
Small game engine written in C++

## Cloning
We use submodules, clone with submodules!

## Building
We use [Premake5](https://premake.github.io/) as a build system. Have this installed.

### Windows
Run `premake5 vs2022` to generate a visual studio solution. This should build out of the box.

### Unix
1. Wayland and X11 are supported. Run `xdg.sh` in the `scripts` directory to generate the required headers.
1. Run `premake5 gmake2` to generate makefiles.


## Dependencies
* [GLFW 3.4](https://github.com/glfw/glfw/tree/3.4)