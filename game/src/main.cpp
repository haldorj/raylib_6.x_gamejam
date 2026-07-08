#include "pch.h"
#include "ui.h"
#include "tilemap.h"

namespace
{
    enum class Mode : uint8_t
    {
        game,
        editorNormal,
        editorAddShape,
    };

    struct Level
    {
        // Level data
        MapTiles tileMap;
        MapTiles tempShape;
        std::vector<std::vector<MapTile>> shapes;
    };

    constexpr auto MaxButtons{static_cast<size_t>(UI::ButtonType::count)};

    struct GameMemory
    {
        Level level;

        // UI ELEMENTS
        std::array<UI::Button, MaxButtons> buttons;

        Music music{};

        Camera2D cameraGame{};
        Camera2D cameraUI{};
        Vector2 mousePosition{};

        // TODO: make arrays for assets
        Texture2D hexagon{};
        Sound fxButton{};

        Mode currentMode{Mode::game};
    };


    std::unique_ptr<GameMemory> Game;
    const auto visibleEditorOnly = [] { return Game->currentMode != Mode::game; };
    const auto visibleShapeModeOnly = [] { return Game->currentMode == Mode::editorAddShape; };

#ifndef PLATFORM_WEB
    bool Running;
#endif

    constexpr const char* ToString(const Mode mode)
    {
        switch (mode)
        {
        case Mode::game: return "GAME";
        case Mode::editorNormal: return "EDITOR";
        case Mode::editorAddShape: return "ADD SHAPE";
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
        Game->fxButton = LoadSound("assets/Sound effects/UI sounds/menu_blip.wav");

        // float timePlayed = 0.0f; // Time played normalized [0.0f..1.0f]
        constexpr float pan = 0.0f; // Default audio pan center [-1.0f..1.0f]
        SetMusicPan(Game->music, pan);
        // Init map
        Game->level.tileMap.Init();
        PlayMusicStream(Game->music);

        auto& shapeButton = Game->buttons.at(static_cast<size_t>(UI::ButtonType::addShape));
        shapeButton.rect = UI::EDITOR_AddShapeButton;
        shapeButton.text = "Add Shape";
        shapeButton.type = UI::ButtonType::addShape;
        shapeButton.onPressed = {
            []
            {
                switch (Game->currentMode)
                {
                case Mode::game: break;
                case Mode::editorNormal:
                    {
                        auto& tempShape{Game->level.tempShape};
                        tempShape.Init();
                        Game->buttons.at(static_cast<size_t>(UI::ButtonType::addShape)).text = "Cancel";
                        Game->currentMode = Mode::editorAddShape;
                    }
                    break;
                case Mode::editorAddShape:
                    {
                        Game->buttons.at(static_cast<size_t>(UI::ButtonType::addShape)).text = "Add Shape";
                        Game->currentMode = Mode::editorNormal;
                    }
                    break;
                }
            }
        };
        shapeButton.isVisible = visibleEditorOnly;

        auto& saveShapeButton = Game->buttons.at(static_cast<size_t>(UI::ButtonType::saveShape));
        saveShapeButton.rect = UI::EDITOR_SaveShapeButton;
        saveShapeButton.text = "Save";
        saveShapeButton.onPressed = {
            []
            {
                if (!Game->level.tempShape.IsEmpty())
                {
                    Game->level.shapes.emplace_back(Game->level.tempShape.ValidTiles());
                }
                
                Game->currentMode = Mode::editorNormal;
            }
        };
        saveShapeButton.isVisible = visibleShapeModeOnly;

#ifndef PLATFORM_WEB
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
                    PlaySound(Game->fxButton);
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
            case Mode::editorAddShape:
                {
                    const MapTile current{PixelToHex(Game->mousePosition)};
                    Game->level.tempShape.SetValid(current.row, current.col);
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
            default: ;
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
    
    void DrawMap(MapTiles& map)
    {
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
                    color = map.At(row, col).isValid ? RED : GREEN;
                    DrawTextureEx(Game->hexagon, pos, 0.f, 1.0f, color);
                }
                if (map.At(row, col).isValid)
                {
                    DrawTextureEx(Game->hexagon, pos, 0.f, 1.0f, color);
                }
            }
        }
    }

    void DrawGameScreen()
    {
        BeginMode2D(Game->cameraGame);

        switch (Game->currentMode)
        {
        case Mode::game:
        case Mode::editorNormal:
            DrawMap(Game->level.tileMap);
            break;
        case Mode::editorAddShape:
            DrawMap(Game->level.tempShape);
            break;
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

        DrawGameScreen();
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
