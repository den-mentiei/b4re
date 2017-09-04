local BUILD_DIR = "./.build"
local PROJECTS_DIR = path.join(BUILD_DIR, "projects")
local TARGET_DIR = path.join(BUILD_DIR, "bin", _TARGET_OS)

require "clang_format"

workspace "b4re"
	configurations { "debug", "release" }
	location(path.join(PROJECTS_DIR, _TARGET_OS))

project "entry"
	kind "WindowedApp"
	language "C"

	targetdir(path.join(TARGET_DIR, "%{cfg.buildcfg}"))

	includedirs { "src", "3rdparty/bgfx/include" }
	libdirs { "3rdparty/bgfx/lib/linux_x64" }

	flags { "FatalWarnings" }
	files { "src/**.h", "src/**.c" }

	includedirs { "3rdparty/curl/include" }
	libdirs { "3rdparty/curl/lib/linux_x64" }

	links { "curlDebug" }

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
		links { 
			"bgfxDebug",
			"bimgDebug",
			"bxDebug",

			"stdc++",
			"m",
			"dl",
			"pthread",

			"X11",
			"GL"
		}
