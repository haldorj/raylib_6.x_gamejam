#include "pch.h"

#include <format>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>      // Emscripten library
#endif

#include <raylib.h>
#include <raymath.h>

namespace hexagon
{
    // Flat top orientation

    struct Hex
    {
        int row;
        int col;
    };

    // Measurements
    constexpr auto Size{8.0};
    [[maybe_unused]] constexpr auto Width{2.0 * Size};
    [[maybe_unused]] constexpr auto Height{std::numbers::sqrt3 * Size};

    // Distances
    [[maybe_unused]] constexpr auto Horizontal{Width * 0.75};
    [[maybe_unused]] constexpr auto Vertical{Height};

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

    static Hex PixelToHex(const Vector2 point)
    {
        const auto x{point.x / Size};
        const auto y{point.y / Size};
        const auto col{2.0 / 3 * x};
        const auto row{-1.0 / 3 * x + std::numbers::sqrt3 / 3 * y};
        const auto result = Hex{
            .row = static_cast<int>(std::round(row)),
            .col = static_cast<int>(std::round(col))
        };
        return result;
    }
}

namespace
{
    constexpr auto WindowWidth{720};
    constexpr auto WindowHeight{720};
    constexpr auto TargetFps{120};
    constexpr auto GamePixelHeight{180};

    Camera2D Camera{};
    Texture2D Hexagon;
    Vector2 MousePosition{};

    void Init()
    {
#ifndef _DEBUG
        SetTraceLogLevel(LOG_NONE); // Disable raylib trace log messages
#endif
        InitWindow(WindowWidth, WindowHeight, "");
        SetTargetFPS(TargetFps);

        Hexagon = LoadTexture("assets/textures/flathex.png");

        Camera.offset = {.x = static_cast<float>(WindowWidth) / 2.f, .y = static_cast<float>(WindowHeight) / 2.f};
        Camera.zoom = static_cast<float>(GetScreenHeight()) / GamePixelHeight;
    }

    void Shutdown()
    {
        CloseWindow();
    }

    void HandleInput()
    {
        MousePosition = Vector2Divide(Vector2Subtract(GetMousePosition(), Camera.offset),
                                      Vector2{Camera.zoom, Camera.zoom});
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

        BeginMode2D(Camera);

        // TODO: Draw your game screen here

        const hexagon::Hex current{hexagon::PixelToHex(MousePosition)};
        for (const auto row : std::views::iota(-3, 4))
        {
            for (const auto col : std::views::iota(-3, 4))
            {
                auto pos{hexagon::HexToPixel(row, col)};
                const auto str{std::format("{}, {}", row, col)};

                pos.x -= hexagon::Size;
                pos.y -= hexagon::Size;

                auto color = WHITE;
                // origin
                if (row == 0 && col == 0)
                {
                    color = YELLOW;
                }

                if (row == current.row && col == current.col)
                {
                    color = GREEN;
                }

                DrawTextureEx(Hexagon, pos, 0.f, 1.0f, color);
            }
        }
        DrawText(std::format("Mouse X{}, Y{}", MousePosition.x, MousePosition.y).c_str(),
                 -GamePixelHeight / 2, -GamePixelHeight / 2, 8, BLUE);

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
    emscripten_set_main_loop(Run, TargetFps, 1);
#else
    while (!WindowShouldClose())
    {
        Run();
    }
#endif
    Shutdown();
}
