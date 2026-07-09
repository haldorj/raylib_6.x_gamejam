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

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>      // Emscripten library
#endif

#include <raylib.h>
#include <raymath.h>
#pragma warning(push, 0)     
#include <raygui.h>
#pragma warning( pop )

constexpr auto WindowWidth{720};
constexpr auto WindowHeight{720};

constexpr auto TargetFps{120};
constexpr auto GamePixelHeight{720};

constexpr auto MinCameraZoom{2.0f};
constexpr auto MaxCameraZoom{8.0f};

constexpr auto HexagonSize{8.0};