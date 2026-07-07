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

struct GameMemory
{
    // Level data
    MapTiles tileMap;

    Camera2D cameraGame{};
    Camera2D cameraUI{};
    Texture2D hexagon{};
    Vector2 mousePosition{};

    Mode currentMode{Mode::game};
};

namespace
{
    constexpr auto WindowWidth{720};
    constexpr auto WindowHeight{720};

    constexpr auto TargetFps{120};
    constexpr auto GamePixelHeight{180};

    constexpr auto MinCameraZoom{2.0f};
    constexpr auto MaxCameraZoom{8.0f};

    constexpr auto HexagonSize{8.0};

    std::unique_ptr<GameMemory> GameMem;

    const char* ToString(const Mode mode)
    {
        switch (mode)
        {
        case Mode::game: return "GAME";
        case Mode::editor: return "EDITOR";
        }
        return "UNKNOWN";
    }

    Vector2 HexToPixel(const int row, const int column)
    {
        // Hex to cartesian
        const auto x{3.0 / 2 * static_cast<double>(column)};
        const auto y{std::numbers::sqrt3 / 2.0 * column + std::numbers::sqrt3 * row};
        // Scale cartesian coordinates
        const auto result = Vector2{
            .x = static_cast<float>(x * HexagonSize),
            .y = static_cast<float>(y * HexagonSize)
        };
        return result;
    }

    MapTile PixelToHex(const Vector2 point)
    {
        const auto x{point.x / HexagonSize};
        const auto y{point.y / HexagonSize};
        const auto col{2.0 / 3 * x};
        const auto row{-1.0 / 3 * x + std::numbers::sqrt3 / 3 * y};
        const auto result = MapTile{
            .row = static_cast<int>(std::round(row)),
            .col = static_cast<int>(std::round(col))
        };
        return result;
    }

    void Init()
    {
#ifndef _DEBUG
        SetTraceLogLevel(LOG_NONE); // Disable raylib trace log messages
#endif
        InitWindow(WindowWidth, WindowHeight, "");
        SetTargetFPS(TargetFps);

        GameMem = std::make_unique<GameMemory>();

        GameMem->hexagon = LoadTexture("assets/textures/flathex.png");

        GameMem->cameraGame.offset = {
            .x = static_cast<float>(WindowWidth) / 2.f,
            .y = static_cast<float>(WindowHeight) / 2.f
        };
        GameMem->cameraGame.zoom = static_cast<float>(GetScreenHeight()) / GamePixelHeight;
        GameMem->cameraUI.zoom = static_cast<float>(GetScreenHeight()) / GamePixelHeight;

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
            GameMem->cameraGame.offset = Vector2Add(GameMem->cameraGame.offset, delta);
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

        const auto newZoom{GameMem->cameraGame.zoom + delta};
        if (newZoom < MinCameraZoom)
        {
            GameMem->cameraGame.zoom = MinCameraZoom;
        }
        else if (newZoom > MaxCameraZoom)
        {
            GameMem->cameraGame.zoom = MaxCameraZoom;
        }
        else
        {
            GameMem->cameraGame.zoom = newZoom;
        }
    }

    void UpdateGame()
    {
        GameMem->mousePosition = Vector2Divide(Vector2Subtract(GetMousePosition(), GameMem->cameraGame.offset),
                                               Vector2{.x = GameMem->cameraGame.zoom, .y = GameMem->cameraGame.zoom});
    }

    void DrawGame()
    {
        BeginMode2D(GameMem->cameraGame);

        const MapTile current{PixelToHex(GameMem->mousePosition)};
        for (const int row : MapTiles::iterator)
        {
            for (const int col : MapTiles::iterator)
            {
                auto pos{HexToPixel(row, col)};

                pos.x -= static_cast<float>(HexagonSize);
                pos.y -= static_cast<float>(HexagonSize);

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

        EndMode2D();
    }

    void DrawUI()
    {
        BeginMode2D(GameMem->cameraUI);

        constexpr auto sideBarWidth{0.8f};
        constexpr auto sideBarHeight{1.0f - sideBarWidth};

        constexpr Rectangle leftSideBar{
            .x = static_cast<float>(GamePixelHeight) * sideBarWidth,
            .y = 0.0f,
            .width = static_cast<float>(GamePixelHeight) * sideBarHeight,
            .height = static_cast<float>(GamePixelHeight)
        };

        constexpr Rectangle bottomSideBar{
            .x = 0.0f,
            .y = static_cast<float>(GamePixelHeight) * sideBarWidth,
            .width = static_cast<float>(GamePixelHeight),
            .height = static_cast<float>(GamePixelHeight) * sideBarHeight
        };

        constexpr auto mergeBarWidth{0.225f};
        constexpr auto mergeBarOffset{1.0f - mergeBarWidth};
        
        constexpr Rectangle mergeWindow{
            .x = static_cast<float>(GamePixelHeight) * mergeBarOffset,
            .y = static_cast<float>(GamePixelHeight) * mergeBarOffset,
            .width = static_cast<float>(GamePixelHeight) * mergeBarWidth,
            .height = static_cast<float>(GamePixelHeight) * mergeBarWidth
        };

        DrawRectangleRec(leftSideBar, GRAY);
        DrawRectangleRec(bottomSideBar, GRAY);
        DrawRectangleRec(mergeWindow, DARKGRAY);

        DrawFPS(0, 0);
        DrawText(std::format("Mode: {}", ToString(GameMem->currentMode)).c_str(),
                 0, 16, 8, BLUE);

        EndMode2D();
    }

    void DrawFrame()
    {
        BeginDrawing();

        ClearBackground(BLACK);

        DrawGame();
        DrawUI();

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
