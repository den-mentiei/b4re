local BUILD_DIR = "./.build"
local PROJECTS_DIR = path.join(BUILD_DIR, "projects")
local TARGET_DIR = path.join(BUILD_DIR, "bin", _TARGET_OS)

workspace "b4re"
	configurations { "debug", "release" }
	location(path.join(PROJECTS_DIR, _TARGET_OS))

project "entry"
	kind "WindowedApp"
	language "C"

	targetdir(path.join(TARGET_DIR, "%{cfg.buildcfg}"))
	

	flags { "FatalWarnings" }
	files { "src/**.h", "src/**.c" }

	filter "configurations:debug"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:release"
		defines { "NDEBUG" }
		optimize "On"

	filter "system:windows"
		defines { "BR_PLATFORM_WIN" }

	filter "system:linux"
		defines { "BR_PLATFORM_LINUX" }
