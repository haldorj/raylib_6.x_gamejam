#include "pch.h"
#include "impl.h"

#ifndef PLATFORM_WEB
	#define RAYGUI_IMPLEMENTATION
	#pragma warning(push, 0)
	#include <raygui.h>
	#pragma warning(pop)
#endif

#ifdef PLATFORM_WEB
	#define RAYGUI_IMPLEMENTATION
	#include <raygui.h>
#endif
