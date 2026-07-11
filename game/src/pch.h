#pragma once

#include <assert.h>
#include <numbers>
#include <ranges>
#include <format>
#include <memory>
#include <array>
#include <functional>
#include <string>
#include <string_view>
#include <print>
#include <optional>
#include <utility>
#include <fstream>
#include <filesystem>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>      // Emscripten library
#endif

#include <raylib.h>
#include <raymath.h>
#ifndef PLATFORM_WEB
    #pragma warning(push, 0)
    #include <raygui.h>
    #pragma warning(pop)
#endif

#ifdef PLATFORM_WEB
    #include <raygui.h>
#endif
