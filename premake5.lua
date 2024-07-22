workspace "hyperengine"
architecture "x86_64"
configurations { "debug", "release", "dist" }
startproject "hyperengine"

flags "MultiProcessorCompile"
language "C++"
cppdialect "C++latest"
cdialect "C17"
staticruntime "On"
stringpooling "On"
editandcontinue "Off"

kind "StaticLib"
targetdir "%{wks.location}/bin/%{cfg.system}_%{cfg.buildcfg}"
objdir "%{wks.location}/bin_obj/%{cfg.system}_%{cfg.buildcfg}"

filter "configurations:debug"
runtime "Debug"
optimize "Debug"
symbols "On"

filter "configurations:release"
runtime "Release"
optimize "Speed"
symbols "On"

filter "configurations:dist"
runtime "Release"
optimize "Speed"
symbols "Off"
flags { "LinkTimeOptimization", "NoBufferSecurityCheck" }

filter "system:windows"
systemversion "latest"
defines "NOMINMAX"
buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus", "/experimental:c11atomics" }

filter { "system:linux", "language:C++" }
buildoptions "-std=c++23"

filter {}

group "dependencies"
for _, matchedfile in ipairs(os.matchfiles("premake/*.lua")) do
    include(matchedfile)
end
group ""

include "hyperengine/build.lua"
include "meshconverter/build.lua"