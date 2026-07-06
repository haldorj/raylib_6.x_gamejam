workspace "game"
	architecture "x64"
	startproject "game"

	configurations
	{
		"Debug",
		"Release",
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
debugdir "%{wks.location}"

IncludeDir = {}
IncludeDir["raylib"] = "%{prj.name}/external/raylib"

project "game"
	location "game"
	kind "ConsoleApp"
	language "C++"
	cppdialect ("C++latest")
	--staticruntime "on"

	conformancemode ("Off")
	exceptionhandling ("Off")
	warnings "Extra"
	fatalwarnings { "All" }
	multiprocessorcompile("On")

	targetdir ("binaries/" .. outputdir .. "/%{prj.name}")
	objdir ("intermediate/" .. outputdir .. "/%{prj.name}")

	pchheader "pch.h"
	pchsource "%{prj.name}/src/pch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.hpp",
		"%{prj.name}/src/**.cpp",
	}

	filter "files:**/external/**"
		enablepch "Off"
	filter {}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{IncludeDir.raylib}/include",
	}

	libdirs 
	{ 
		"%{IncludeDir.raylib}/lib/",
	}

	filter "system:windows"
	systemversion "latest"
	links 
	{ 
		"raylib.lib",
		"winmm.lib",
	}

	filter "configurations:Debug"
		defines "GAME_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "GAME_RELEASE"
		runtime "Release"
		optimize "on"
