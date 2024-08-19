project "bullet3"
location "%{wks.location}/vendor/%{prj.name}"

warnings "Off"

files {
	"%{prj.location}/src/**.cpp",
	"%{prj.location}/src/**.h"
}

removefiles {
	"%{prj.location}/src/clew/**.cpp",
	"%{prj.location}/src/clew/**.h",
	"%{prj.location}/src/Bullet3OpenCL/**.cpp",
	"%{prj.location}/src/Bullet3OpenCL/**.h",
	"%{prj.location}/src/*.cpp"
}

includedirs "%{prj.location}/src"