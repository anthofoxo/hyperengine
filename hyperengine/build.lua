project "hyperengine"
debugdir "../working"
kind "ConsoleApp"

defines { "GLFW_INCLUDE_NONE", "TRACY_ENABLE" }

files {
    "%{prj.location}/**.cpp",
    "%{prj.location}/**.cc",
    "%{prj.location}/**.c",
    "%{prj.location}/**.hpp",
    "%{prj.location}/**.h",
    "%{prj.location}/**.inl",
}

includedirs {
    "%{prj.location}",
    "%{prj.location}/source",
    "%{prj.location}/vendor",
    "%{wks.location}/vendor/glfw/include",
    "%{wks.location}/vendor/glad/include",
    "%{wks.location}/vendor/glm",
    "%{wks.location}/vendor/imgui",
    "%{wks.location}/vendor/imguizmo",
    "%{wks.location}/vendor/assimp_config",
	"%{wks.location}/vendor/assimp/include",
    "%{wks.location}/vendor/entt/src",
    "%{wks.location}/vendor/lua/src",
    "%{wks.location}/vendor/tracy/public",
    "%{wks.location}/vendor/spdlog/include",
    "%{wks.location}/vendor/bullet3/src",
}

links { "glfw", "glad", "imgui", "imguizmo", "assimp", "lua", "tracy", "bullet3" }

filter "system:windows"
files "%{prj.location}/*.rc"
links "opengl32"

filter { "configurations:dist", "system:windows" }
kind "WindowedApp"
defines "HE_ENTRY_WINMAIN"

filter "system:linux"
links { "pthread", "dl" }