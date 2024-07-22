project "meshconverter"
debugdir "../working"
kind "ConsoleApp"

files "%{prj.location}/**.cpp"

includedirs {
    "%{prj.location}/source",
    "%{wks.location}/vendor/assimp_config",
	"%{wks.location}/vendor/assimp/include",
}

links "assimp"