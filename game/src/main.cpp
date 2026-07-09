#include "pch.h"

#include <print>

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

    using shape = std::vector<MapTile>;

    // TODO: support saving and loading this struct
    struct Level
    {
        // Level data
        MapTiles tileMap; // Main level
        MapTiles tempShape; // Shape editor level
        std::vector<std::pair<shape, RenderTexture>> shapes;
    };

    struct GameMemory
    {
        Level level;

        Music music{};

        Camera2D cameraGame{};
        Camera2D cameraUI{};
        Vector2 mousePosition{};

        // TODO: make arrays for assets
        Texture2D hexagon{};
        Sound fxButton{};

        Mode currentMode{Mode::game};
        bool showMessageBox{false};
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

    RenderTexture GeneratePreviewTexture(const std::span<MapTile> shape)
    {
        const RenderTexture renderTexture = LoadRenderTexture(GetScreenHeight(), GetScreenWidth());

        Camera2D previewCamera{};
        previewCamera.offset = Vector2{
            .x = static_cast<float>(GetScreenHeight()) / 2.0f,
            .y = static_cast<float>(GetScreenWidth()) / 2.0f
        };
        previewCamera.zoom = 6.0f;

        BeginDrawing();
        BeginTextureMode(renderTexture);
        BeginMode2D(previewCamera);
        ClearBackground(BLACK);
        for (const auto& hex : shape)
        {
            auto pos{HexToPixel(hex.row, hex.col)};
            pos.x -= static_cast<float>(HexagonSize);
            pos.y -= static_cast<float>(HexagonSize);
            DrawTextureEx(Game->hexagon, pos, 0.f, 1.0f, WHITE);
        }
        EndTextureMode();
        EndDrawing();
        SetTextureFilter(renderTexture.texture, TEXTURE_FILTER_BILINEAR);
        return renderTexture;
    }

    void AddShape()
    {
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
        Game->cameraUI.zoom = 1.0f; // no scale;

        Game->music = LoadMusicStream("assets/Music/Goblins_Dance_(Battle).wav");
        Game->fxButton = LoadSound("assets/Sound effects/UI sounds/menu_blip.wav");

        // float timePlayed = 0.0f; // Time played normalized [0.0f..1.0f]
        constexpr float pan = 0.0f; // Default audio pan center [-1.0f..1.0f]
        SetMusicPan(Game->music, pan);
        // Init map
        Game->level.tileMap.Init();
        PlayMusicStream(Game->music);

#ifndef PLATFORM_WEB
        Running = true;
#endif
    }

    void HandleInput()
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (Game->showMessageBox)
            {
                return;
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

    void RenderMap(MapTiles& map)
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

    void RenderGameScreen()
    {
        BeginMode2D(Game->cameraGame);

        switch (Game->currentMode)
        {
        case Mode::game:
        case Mode::editorNormal:
            RenderMap(Game->level.tileMap);
            break;
        case Mode::editorAddShape:
            RenderMap(Game->level.tempShape);
            break;
        }
        EndMode2D();
    }

    void RenderUI()
    {
        BeginMode2D(Game->cameraUI);

        DrawRectangleRec(UI::LeftSideBar, LIGHTGRAY);
        DrawRectangleRec(UI::BottomSideBar, LIGHTGRAY);
        DrawRectangleRec(UI::MergeWindow, DARKGRAY);

        auto textPositionY{0};
        if (Game->currentMode == Mode::editorNormal)
        {
            DrawFPS(0, 0);
            textPositionY += 16;
        }
        float index{0};
        for (const auto& tex : Game->level.shapes | std::views::values)
        {
            const Rectangle source = {
                .x = 0,
                .y = 0,
                .width = static_cast<float>(tex.texture.width),
                .height = -static_cast<float>(tex.texture.height)
            };

            constexpr auto scaleFactor{0.9f};
            constexpr auto indentValue{UI::LeftSideBar.width * (1.0f - scaleFactor) * 0.5f};
            float initialOffset{indentValue};
            if (Game->currentMode != Mode::game)
            {
                initialOffset = 40.f;
            }

            const Rectangle dest = {
                .x = UI::LeftSideBar.x + indentValue,
                .y = UI::LeftSideBar.y + initialOffset + (index * UI::LeftSideBar.width),
                .width = UI::LeftSideBar.width * scaleFactor,
                .height = UI::LeftSideBar.width * scaleFactor,
            };
            DrawTexturePro(tex.texture, source, dest, Vector2{.x = 0, .y = 0}, 0.0f, WHITE);
            index++;
        }

        switch (Game->currentMode)
        {
        case Mode::game:
            {
            }
            break;
        case Mode::editorNormal:
            {
                if (GuiButton(UI::EDITOR_AddShapeButton, "Add Shape") > 0)
                {
                    Game->level.tempShape.Init();
                    Game->currentMode = Mode::editorAddShape;
                }
            }
            break;
        case Mode::editorAddShape:
            {
                if (GuiButton(UI::EDITOR_AddShapeButton, "Cancel") > 0)
                {
                    Game->currentMode = Mode::editorNormal;
                }
                if (GuiButton(UI::EDITOR_SaveShapeButton, "Save"))
                {
                    Game->showMessageBox = true;
                }
                if (Game->showMessageBox)
                {
                    const auto result{
                        GuiMessageBox(UI::MessageBox,
                                      "Save",
                                      "Do you want to save the current shape?",
                                      "YES;NO")
                    };
                    if (result == 1) //YES
                    {
                        if (!Game->level.tempShape.IsEmpty())
                        {
                            auto newShape{Game->level.tempShape.ValidTiles()};
                            const auto previewTexture{GeneratePreviewTexture(newShape)};
                            Game->level.shapes.emplace_back(newShape, previewTexture);
                        }
                        Game->showMessageBox = false;
                        Game->currentMode = Mode::editorNormal;
                    }
                    if (result == 2) //NO
                    {
                        Game->showMessageBox = false;
                    }
                }
            }
            break;
        }
        DrawText(std::format("Mode: {}", ToString(Game->currentMode)).c_str(),
                 0, textPositionY, 8, BLUE);

        EndMode2D();
    }

    void RenderFrame()
    {
        BeginDrawing();

        ClearBackground(BLACK);

        RenderGameScreen();
        RenderUI();

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
        RenderFrame();
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
