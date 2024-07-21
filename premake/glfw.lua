project "glfw"

location "%{wks.location}/vendor/%{prj.name}"
language "C"
defines "_GLFW_USE_HYBRID_HPG"

files {
    "%{prj.location}/src/*.c",
    "%{prj.location}/src/*.h"
}

filter "system:windows"
defines { "_GLFW_WIN32", "_CRT_SECURE_NO_WARNINGS" }

filter "system:linux"
defines { "_GLFW_X11", "_GLFW_WAYLAND" }
-- Temporary fix for `linux_joystick.c:138:51: In function 'openJoystickDevice': error: 'O_CLOEXEC' undeclared`
defines "_XOPEN_SOURCE=700"

filter "system:macosx"
defines "_GLFW_COCOA"