#include "pch.h"
#include "ui.h"
#include "tilemap.h"

enum class Mode : uint8_t
{
    game,
    editorNormal,
};

struct Level
{
    // Level data
    MapTiles tileMap;
};

struct GameMemory
{
    Level level;

    // UI ELEMENTS
    std::vector<UI::Button> buttons;

    Music music{};
    Camera2D cameraGame{};
    Camera2D cameraUI{};
    Texture2D hexagon{};
    Vector2 mousePosition{};

    Mode currentMode{Mode::game};
};

namespace
{
    std::unique_ptr<GameMemory> Game;
#ifndef PLATFORM_WEB
    bool Running;
#endif

    constexpr const char* ToString(const Mode mode)
    {
        switch (mode)
        {
        case Mode::game: return "GAME";
        case Mode::editorNormal: return "EDITOR";
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

    void Shutdown()
    {
        CloseWindow();
    }

    void Init()
    {
#ifndef _DEBUG
        SetTraceLogLevel(LOG_NONE); // Disable raylib trace log messages
#endif
        InitWindow(WindowWidth, WindowHeight, "");
        SetTargetFPS(TargetFps);
        InitAudioDevice();

        Game = std::make_unique<GameMemory>();
        Game->hexagon = LoadTexture("assets/textures/flathex.png");

        // Game Camera
        Game->cameraGame.offset = UI::CameraStartPositionRelativeToUI;
        // UI Camera
        Game->cameraGame.zoom = static_cast<float>(GetScreenHeight()) / GamePixelHeight;
        Game->cameraUI.zoom = static_cast<float>(GetScreenHeight()) / GamePixelHeight;

        Game->music = LoadMusicStream("assets/Music/Goblins_Dance_(Battle).wav");

        // float timePlayed = 0.0f; // Time played normalized [0.0f..1.0f]
        constexpr float pan = 0.0f; // Default audio pan center [-1.0f..1.0f]
        SetMusicPan(Game->music, pan);
        // Init map
        Game->level.tileMap.Init();
        PlayMusicStream(Game->music);
        
        // DESKTOP ONLY
#ifndef PLATFORM_WEB
        UI::Button quitButton;
        quitButton.rect = UI::EDITOR_AddShapeButton;
        quitButton.text = "Quit";
        quitButton.onPressed = {[] { Running = false; }};
        Game->buttons.emplace_back(quitButton);
        Running = true;
#endif
    }

    void HandleInput()
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            for (const auto& button : Game->buttons)
            {
                if (::UI::IsButtonHovered(button))
                {
                    button.onPressed();
                }
            }

            switch (Game->currentMode)
            {
            case Mode::game:
                {
                }
                break;
            case Mode::editorNormal:
                {
                    const MapTile current{PixelToHex(Game->mousePosition)};
                    Game->level.tileMap.SetValid(current.row, current.col);
                }
                break;
            }
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            const auto delta{GetMouseDelta()};
            Game->cameraGame.offset = Vector2Add(Game->cameraGame.offset, delta);
        }

#ifdef _DEBUG
        if (IsKeyPressed(KEY_F2))
        {
            switch (Game->currentMode)
            {
            case Mode::game:
                Game->currentMode = Mode::editorNormal;
                break;
            case Mode::editorNormal:
                Game->currentMode = Mode::game;
                break;
            }
        }
#endif

        const auto delta{GetMouseWheelMove()};

        if (const auto newZoom{Game->cameraGame.zoom + delta};
            newZoom < MinCameraZoom)
        {
            Game->cameraGame.zoom = MinCameraZoom;
        }
        else if (newZoom > MaxCameraZoom)
        {
            Game->cameraGame.zoom = MaxCameraZoom;
        }
        else
        {
            Game->cameraGame.zoom = newZoom;
        }
    }

    void UpdateGame()
    {
        Game->mousePosition = Vector2Divide(Vector2Subtract(GetMousePosition(), Game->cameraGame.offset),
                                            Vector2{.x = Game->cameraGame.zoom, .y = Game->cameraGame.zoom});

        UpdateMusicStream(Game->music); // Update music buffer with new stream data
    }

    void DrawGame()
    {
        BeginMode2D(Game->cameraGame);

        const MapTile current{PixelToHex(Game->mousePosition)};
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
                    color = Game->level.tileMap.At(row, col).isValid ? RED : GREEN;
                    DrawTextureEx(Game->hexagon, pos, 0.f, 1.0f, color);
                }
                if (Game->level.tileMap.At(row, col).isValid)
                {
                    DrawTextureEx(Game->hexagon, pos, 0.f, 1.0f, color);
                    //DrawText(std::format("R{}, Q{}", row, col).c_str(), static_cast<int>(pos.x), static_cast<int>(pos.y), 1, GREEN);
                }
            }
        }

        EndMode2D();
    }

    void DrawUI()
    {
        BeginMode2D(Game->cameraUI);

        DrawRectangleRec(UI::LeftSideBar, LIGHTGRAY);
        DrawRectangleRec(UI::BottomSideBar, LIGHTGRAY);
        DrawRectangleRec(UI::MergeWindow, DARKGRAY);

        for (const auto& button : Game->buttons)
        {
            UI::RenderButton(button);
        }

        auto textPositionY{0};
        if (Game->currentMode == Mode::editorNormal)
        {
            DrawFPS(0, 0);
            textPositionY += 16;
        }

        DrawText(std::format("Mode: {}", ToString(Game->currentMode)).c_str(),
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
        if (!Game)
        {
            return;
        }

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
    while (!WindowShouldClose() && Running)
    {
        Run();
    }
#endif
    Shutdown();
}
