#include "pch.h"
#include "ui.h"
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
    Music music{};
    Camera2D cameraGame{};
    Camera2D cameraUI{};
    Texture2D hexagon{};
    Vector2 mousePosition{};

    // UI ELEMENTS
    std::vector<UI::Button> buttons;

    Mode currentMode{Mode::game};
};

namespace
{
    std::unique_ptr<GameMemory> GameMem;

    constexpr const char* ToString(const Mode mode)
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
        InitAudioDevice();

        GameMem = std::make_unique<GameMemory>();
        GameMem->hexagon = LoadTexture("assets/textures/flathex.png");

        // Game Camera
        GameMem->cameraGame.offset = UI::CameraStartPositionRelativeToUI;
        // UI Camera
        GameMem->cameraGame.zoom = static_cast<float>(GetScreenHeight()) / GamePixelHeight;
        GameMem->cameraUI.zoom = static_cast<float>(GetScreenHeight()) / GamePixelHeight;

        GameMem->music = LoadMusicStream("assets/Music/Goblins_Dance_(Battle).wav");

        // float timePlayed = 0.0f; // Time played normalized [0.0f..1.0f]
        constexpr float pan = 0.0f; // Default audio pan center [-1.0f..1.0f]
        SetMusicPan(GameMem->music, pan);
        // Init map
        GameMem->tileMap.Init();
        PlayMusicStream(GameMem->music);
    }

    void Shutdown()
    {
        CloseWindow();
    }

    void HandleInput()
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            for (const auto& button : GameMem->buttons)
            {
                if (::UI::IsButtonHovered(button))
                {
                    button.onPressed();
                }
            }

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

        if (const auto newZoom{GameMem->cameraGame.zoom + delta};
            newZoom < MinCameraZoom)
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

        UpdateMusicStream(GameMem->music); // Update music buffer with new stream data
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

        DrawRectangleRec(UI::LeftSideBar, GRAY);
        DrawRectangleRec(UI::BottomSideBar, GRAY);
        DrawRectangleRec(UI::MergeWindow, DARKGRAY);

        auto textPositionY{0};
        if (GameMem->currentMode == Mode::editor)
        {
            DrawFPS(0, 0);
            textPositionY += 16;
        }

        DrawText(std::format("Mode: {}", ToString(GameMem->currentMode)).c_str(),
                 0, textPositionY, 8, BLUE);

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
