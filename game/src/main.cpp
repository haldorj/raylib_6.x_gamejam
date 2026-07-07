#include "pch.h"

#include <charconv>
#include <ios>
#include <ranges>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>      // Emscripten library
#endif

#include <raylib.h>

namespace hexagon
{
    // Pointy top orientation

    // Measurements
    constexpr auto Size{8};
    constexpr auto Width{static_cast<int>(std::numbers::sqrt3 * Size)};
    constexpr auto Height{2 * Size};
    // Distances
    constexpr auto Horizontal{static_cast<int>(Width)};
    constexpr auto Vertical{static_cast<int>(Height * 0.75)};

    Vector2 HexToPixel(const int row, const int column)
    {
        const auto even{row % 2 == 1};
        const auto x = std::numbers::sqrt3 * (column + (even ? 0.5 : 0.0));
        const auto y = (3.0 / 2 * row);
        // scale cartesian coordinates
        const auto result{Vector2{static_cast<float>(x * Size), static_cast<float>(y * Size)}};
        return result;
    }

    constexpr Vector2 Points[] = {
        Vector2{.x = static_cast<float>(Width / 2.0), .y = Height},
        Vector2{.x = static_cast<float>(Width), .y = Height * 0.75},
        Vector2{.x = static_cast<float>(Width), .y = Height * 0.25},
        Vector2{.x = static_cast<float>(Width / 2), .y = 0},
        Vector2{.x = 0, .y = Height * 0.25},
        Vector2{.x = 0, .y = Height * 0.75},
    };
}

namespace
{
    constexpr auto WindowWidth{720};
    constexpr auto WindowHeight{720};
    constexpr auto TargetFps{60};
    constexpr auto GamePixelHeight{180};

    Camera2D camera{};

    Texture2D hexagon;

    RenderTexture2D renderTexture;

    void Init()
    {
#ifndef _DEBUG
        SetTraceLogLevel(LOG_NONE); // Disable raylib trace log messages
#endif
        InitWindow(WindowWidth, WindowHeight, "raylib gamejam template");
        SetTargetFPS(TargetFps);

        hexagon = LoadTexture("assets/textures/hex.png");

        camera.zoom = static_cast<float>(GetScreenHeight()) / GamePixelHeight;

        renderTexture = LoadRenderTexture(WindowWidth, WindowHeight);
        SetTextureFilter(renderTexture.texture, TEXTURE_FILTER_POINT);
    }

    void Shutdown()
    {
        UnloadRenderTexture(renderTexture);
        CloseWindow();
    }

    void HandleInput()
    {
    }

    void UpdateGame()
    {
    }

    void DrawFrame()
    {
        //frameCounter++;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        // Render game screen to a texture, 
        // it could be useful for scaling or further shader postprocessing
        BeginTextureMode(renderTexture);
        ClearBackground(SKYBLUE);

        // TODO: Draw your game screen here

        // const auto drawHexgon = [](const int centerX, const int centerY, const Color color)
        // {
        //     for (size_t startIndex = 0; startIndex < std::size(hexagon::Points); ++startIndex)
        //     {
        //         size_t endIndex = {startIndex + 1};
        //         if (startIndex + 1 >= std::size(hexagon::Points))
        //         {
        //             endIndex = 0;
        //         }
        //         const auto& [startX, startY] = hexagon::Points[startIndex];
        //         const auto& [endX, endY] = hexagon::Points[endIndex];
        //
        //         DrawLine(centerX + static_cast<int>(startX), centerY + static_cast<int>(startY),
        //                  centerX + static_cast<int>(endX), centerY + static_cast<int>(endY),
        //                  color);
        //     }
        // };

        for (const auto row : std::views::iota(0, 12))
        {
            for (const auto col : std::views::iota(0, 14))
            {
                DrawTextureEx(hexagon, hexagon::HexToPixel(row, col), 0.f, 1.0f, WHITE);
            }
        }

        DrawText("HEXAGONS", 10, 10, 10, DARKGRAY);

        //
        EndTextureMode();

        // Render to screen (main framebuffer)
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

        // Draw render texture to screen, scaled if required
        const auto source{
            Rectangle{
                .x = 0,
                .y = 0,
                .width = static_cast<float>(renderTexture.texture.width),
                .height = -static_cast<float>(renderTexture.texture.height)
            }
        };
        const auto dest{
            Rectangle{
                .x = 0, .y = 0,
                .width = static_cast<float>(renderTexture.texture.width),
                .height = static_cast<float>(renderTexture.texture.height)
            }
        };
        constexpr auto origin{Vector2{.x = 0, .y = 0}};
        constexpr auto rotation{0.0f};
        DrawTexturePro(renderTexture.texture, source, dest, origin, rotation, WHITE);

        EndMode2D();

        // TODO: Draw everything that requires to be drawn at this point, maybe UI?
        EndDrawing();
    }

    //main game loop
    void Run()
    {
        HandleInput();
        UpdateGame();
        DrawFrame();
    }
}

int main()
{
    Init();
#ifdef PLATFORM_WEB
    constexpr auto GameUpdateRateWEB{60};
    emscripten_set_main_loop(Run, GameUpdateRateWEB, 1);
#else
    while (!WindowShouldClose())
    {
        Run();
    }
#endif
    Shutdown();
}
