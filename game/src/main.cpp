#include "pch.h"

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>      // Emscripten library
#endif

#include <raylib.h>
#include <raymath.h>

#include "tilemap.h"

enum class Mode : uint8_t
{
    game,
    editor,
};

const char* ToString(const Mode mode)
{
    switch (mode)
    {
    case Mode::game: return "GAME";
    case Mode::editor: return "EDITOR";
    }
    return "UNKNOWN";
}

struct Entity
{
    // position in grid
    int col;
    int row;
};

struct GameMemory
{
    // Level data
    MapTiles tileMap;

    Camera2D camera{};
    Texture2D hexagon{};
    Vector2 mousePosition{};

    Mode currentMode{Mode::game};
};

namespace hexagon
{
    constexpr auto Size{8.0};
}

namespace
{
    Vector2 HexToPixel(const int row, const int column)
    {
        // Hex to cartesian
        const auto x{3.0 / 2 * static_cast<double>(column)};
        const auto y{std::numbers::sqrt3 / 2.0 * column + std::numbers::sqrt3 * row};
        // Scale cartesian coordinates
        const auto result = Vector2{
            .x = static_cast<float>(x * hexagon::Size),
            .y = static_cast<float>(y * hexagon::Size)
        };
        return result;
    }

    MapTile PixelToHex(const Vector2 point)
    {
        const auto x{point.x / hexagon::Size};
        const auto y{point.y / hexagon::Size};
        const auto col{2.0 / 3 * x};
        const auto row{-1.0 / 3 * x + std::numbers::sqrt3 / 3 * y};
        const auto result = MapTile{
            .row = static_cast<int>(std::round(row)),
            .col = static_cast<int>(std::round(col))
        };
        return result;
    }

    constexpr auto WindowWidth{720};
    constexpr auto WindowHeight{720};

    constexpr auto TargetFps{120};
    constexpr auto GamePixelHeight{180};

    constexpr auto MinCameraZoom{2.0f};
    constexpr auto MaxCameraZoom{8.0f};

    std::unique_ptr<GameMemory> GameMem;

    void Init()
    {
#ifndef _DEBUG
        SetTraceLogLevel(LOG_NONE); // Disable raylib trace log messages
#endif
        InitWindow(WindowWidth, WindowHeight, "");
        SetTargetFPS(TargetFps);

        GameMem = std::make_unique<GameMemory>();

        GameMem->hexagon = LoadTexture("assets/textures/flathex.png");

        GameMem->camera.offset = {
            .x = static_cast<float>(WindowWidth) / 2.f,
            .y = static_cast<float>(WindowHeight) / 2.f
        };
        GameMem->camera.zoom = static_cast<float>(GetScreenHeight()) / GamePixelHeight;

        // Init map
        GameMem->tileMap.Init();
    }

    void Shutdown()
    {
        CloseWindow();
    }

    void HandleInput()
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            switch (GameMem->currentMode)
            {
            case Mode::game:
                {
                }
                break;
            case Mode::editor:
                {
                    const MapTile current{PixelToHex(GameMem->mousePosition)};
                    GameMem->tileMap.SetValid(current.row, current.col);
                }
                break;
            }
        }
        
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            const auto delta{GetMouseDelta()};
            GameMem->camera.offset = Vector2Add(GameMem->camera.offset, delta);
        }

#ifdef _DEBUG
        if (IsKeyPressed(KEY_F2))
        {
            switch (GameMem->currentMode)
            {
            case Mode::game:
                GameMem->currentMode = Mode::editor;
                break;
            case Mode::editor:
                GameMem->currentMode = Mode::game;
                break;
            }
        }
#endif

        const auto delta{GetMouseWheelMove()};

        const auto newZoom{GameMem->camera.zoom + delta};
        if (newZoom < MinCameraZoom)
        {
            GameMem->camera.zoom = MinCameraZoom;
        }
        else if (newZoom > MaxCameraZoom)
        {
            GameMem->camera.zoom = MaxCameraZoom;
        }
        else
        {
            GameMem->camera.zoom = newZoom;
        }
    }

    void UpdateGame()
    {
        GameMem->mousePosition = Vector2Divide(Vector2Subtract(GetMousePosition(), GameMem->camera.offset),
                                               Vector2{.x = GameMem->camera.zoom, .y = GameMem->camera.zoom});
    }

    void DrawFrame()
    {
        BeginDrawing();

        ClearBackground(BLACK);
        BeginMode2D(GameMem->camera);

        const MapTile current{PixelToHex(GameMem->mousePosition)};
        for (const int row : MapTiles::iterator)
        {
            for (const int col : MapTiles::iterator)
            {
                auto pos{HexToPixel(row, col)};

                pos.x -= static_cast<float>(hexagon::Size);
                pos.y -= static_cast<float>(hexagon::Size);

                auto color = WHITE;
                if (row == current.row && col == current.col)
                {
                    color = GameMem->tileMap.At(row, col).isValid ? RED : GREEN;
                    DrawTextureEx(GameMem->hexagon, pos, 0.f, 1.0f, color);
                }
                if (GameMem->tileMap.At(row, col).isValid)
                {
                    DrawTextureEx(GameMem->hexagon, pos, 0.f, 1.0f, color);
                    //DrawText(std::format("R{}, Q{}", row, col).c_str(), static_cast<int>(pos.x), static_cast<int>(pos.y), 1, GREEN);
                }
            }
        }
        DrawFPS(-GamePixelHeight / 2, -GamePixelHeight / 2);
        DrawText(std::format("Mouse X{}, Y{}", GameMem->mousePosition.x, GameMem->mousePosition.y).c_str(),
                 -GamePixelHeight / 2, 16 - GamePixelHeight / 2, 8, BLUE);
        DrawText(ToString(GameMem->currentMode),
                 -GamePixelHeight / 2, 24 - GamePixelHeight / 2, 8, BLUE);
        
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
