#include "pch.h"

#include <format>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>      // Emscripten library
#endif

#include <raylib.h>

namespace hexagon
{
    // Flat top orientation

    // Measurements
    constexpr auto Size{8.0};
    constexpr auto Width{2.0 * Size};
    constexpr auto Height{std::numbers::sqrt3 * Size};

    // Distances
    constexpr auto Horizontal{Width * 0.75};
    constexpr auto Vertical{Height};

    static Vector2 HexToPixel(const int row, const int column)
    {
        // Hex to cartesian
        const auto x{3.0 / 2 * static_cast<double>(column)};
        const auto y{std::numbers::sqrt3 / 2.0 * column + std::numbers::sqrt3 * row};
        // Scale cartesian coordinates
        const auto result = Vector2{
            .x = static_cast<float>(x * Size),
            .y = static_cast<float>(y * Size)
        };
        return result; 
    }

    // constexpr Vector2 Points[] = {
    //     Vector2{.x = static_cast<float>(Width / 2.0), .y = Height},
    //     Vector2{.x = static_cast<float>(Width), .y = Height * 0.75},
    //     Vector2{.x = static_cast<float>(Width), .y = Height * 0.25},
    //     Vector2{.x = static_cast<float>(Width / 2), .y = 0},
    //     Vector2{.x = 0, .y = Height * 0.25},
    //     Vector2{.x = 0, .y = Height * 0.75},
    // };
}

namespace
{
    constexpr auto WindowWidth{720};
    constexpr auto WindowHeight{720};
    constexpr auto TargetFps{60};
    constexpr auto GamePixelHeight{180};

    Camera2D camera{};

    Texture2D hexagon;

    void Init()
    {
#ifndef _DEBUG
        SetTraceLogLevel(LOG_NONE); // Disable raylib trace log messages
#endif
        InitWindow(WindowWidth, WindowHeight, "raylib gamejam template");
        SetTargetFPS(TargetFps);

        hexagon = LoadTexture("assets/textures/flathex.png");
        //hexagon = LoadTexture("assets/textures/Object/explosive directional.png");
        //hexagon = LoadTexture("assets/textures/Object/explosive.png");
       // hexagon = LoadTexture("assets/textures/Object/Sprite-0003.png");


        camera.offset = {.x = static_cast<float>(WindowWidth) / 2.f, .y = static_cast<float>(WindowHeight) / 2.f};
        camera.zoom = static_cast<float>(GetScreenHeight()) / GamePixelHeight;
    }

    void Shutdown()
    {
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
        //BeginTextureMode(renderTexture);

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(camera);
        
        // TODO: Draw your game screen here

        for (const auto row : std::views::iota(-5, 5))
        {
            for (const auto col : std::views::iota(-5, 5))
            {
                const auto pos{hexagon::HexToPixel(row, col)};
                const auto str{std::format("{}, {}", row, col)};
                DrawTextureEx(hexagon, pos, 0.f, 1.0f, WHITE);
                //DrawText(str.c_str(), static_cast<int>(pos.x), static_cast<int>(pos.y), 1, RED);
            }
        }

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
