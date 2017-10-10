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

	flags { "FatalWarnings" }

	files {
		"src/**.h", "src/**.c",
		"3rdparty/tinycthread/*.c"
	}

	includedirs { "src" }

	sysincludedirs {
		"3rdparty/bgfx/include",
		"3rdparty/curl/include",
		"3rdparty/tinycthread"
	}

	links {
		"curlDebug",
		"bgfxRelease",
		"bimgRelease",
		"bxRelease",
	}

	filter "configurations:debug"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:release"
		defines { "NDEBUG" }
		optimize "On"

	filter "system:windows"
		defines { "BR_PLATFORM_WIN" }

	filter "system:macosx"
		defines { "BR_PLATFORM_MACOS" }

		libdirs {
			"3rdparty/bgfx/lib/osx_x64",
			"3rdparty/curl/lib/macosx_x64"
		}

		files { "src/**.m", "src/Info.plist" }
	
		links {
			"stdc++",
			"objc",

			"Foundation.framework",
			"Security.framework",
			"AppKit.framework",
			"QuartzCore.framework",
			"Metal.framework"
		}

	filter "system:linux"
		defines { "BR_PLATFORM_LINUX" }

		libdirs {
			"3rdparty/curl/lib/linux_x64",
			"3rdparty/bgfx/lib/linux_x64"
		}

		links { 
			"stdc++",
			"m",
			"dl",
			"pthread",

			"X11",
			"GL"
		}
