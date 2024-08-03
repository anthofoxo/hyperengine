project "imguizmo"

location "%{wks.location}/vendor/%{prj.name}"
language "C++"

defines "IMGUI_DEFINE_MATH_OPERATORS"

files {
	"%{prj.location}/*.cpp",
	"%{prj.location}/*.h"
}

removefiles {
	"%{prj.location}/ImSequencer.cpp",
	"%{prj.location}/ImSequencer.h",
	"%{prj.location}/GraphEditor.cpp",
	"%{prj.location}/GraphEditor.h"
}

includedirs "%{wks.location}/vendor/imgui"