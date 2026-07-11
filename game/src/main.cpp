#include "pch.h"

#include "ui.h"
#include "tilemap.h"
#include "game.h"

#include <fstream>
#include <filesystem>


namespace
{
    std::unique_ptr<GameMemory> Game;
#ifndef PLATFORM_WEB
    bool Running;
#endif

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
            .row = static_cast<int8_t>(std::round(row)),
            .col = static_cast<int8_t>(std::round(col))
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
        if (!Game->level.tempShape.IsEmpty())
        {
            auto newShape{Game->level.tempShape.ValidTiles()};
            const auto previewTexture{GeneratePreviewTexture(newShape)};
            Game->level.shapes.emplace_back(newShape, previewTexture);
        }
    }

    void RemoveShapeAtIndex(const size_t index)
    {
        if (auto& shapes{Game->level.shapes}; index < shapes.size())
        {
            UnloadRenderTexture(shapes.at(index).second);
            shapes.erase(shapes.begin() + static_cast<int>(index));
        }
    }

    void AddSpell(const Spell spell)
    {
        Game->level.spells.push_back(spell);
    }
    void SaveLevel(const std::string& filename)
    {
        std::filesystem::create_directories("saves");
        std::ofstream file("saves/" + filename);
        if (!file)
        {
            std::println("Kunne ikke åpne fil for lagring: {}", filename);
            return;
        }

        // TileMap
        const auto tiles{Game->level.tileMap.ValidTiles()};
        file << "TILEMAP " << tiles.size() << "\n";
        for (const auto& t : tiles)
        {
            file << static_cast<int>(t.row) << " " << static_cast<int>(t.col) << "\n";
        }

        // Shapes (kun rådata, ikke teksturen)
        file << "SHAPES " << Game->level.shapes.size() << "\n";
        for (const auto& [shape, tex] : Game->level.shapes)
        {
            file << shape.size() << "\n";
            for (const auto& t : shape)
            {
                file << static_cast<int>(t.row) << " " << static_cast<int>(t.col) << "\n";
            }
        }

        // Spells
        file << "SPELLS " << Game->level.spells.size() << "\n";
        for (const auto& s : Game->level.spells)
        {
            file << static_cast<int>(s) << "\n";
        }

        std::println("Level lagret: saves/{}", filename);
    }
    void LoadLevel(const std::string& filename)
        {
            std::ifstream file("saves/" + filename);
            if (!file)
            {
                std::println("Fant ikke lagret fil: {}", filename);
                return;
            }
    
            // Rydd opp gammel state først
            for (auto& [shape, tex] : Game->level.shapes)
            {
                UnloadRenderTexture(tex);
            }
            Game->level.shapes.clear();
            Game->level.spells.clear();
            Game->level.tileMap.Init(); // nullstill tilemap
    
            std::string tag;
            size_t count{};
    
            // TileMap
            file >> tag >> count;
            for (size_t i{0}; i < count; ++i)
            {
                int row{}, col{};
                file >> row >> col;
                Game->level.tileMap.SetValid(row, col);
            }
    
            // Shapes
            file >> tag >> count;
            for (size_t i{0}; i < count; ++i)
            {
                size_t shapeSize{};
                file >> shapeSize;
    
                std::vector<MapTile> shape;
                shape.reserve(shapeSize);
                for (size_t j{0}; j < shapeSize; ++j)
                {
                    int row{}, col{};
                    file >> row >> col;
                    shape.push_back(MapTile{
                        .row = static_cast<int8_t>(row),
                        .col = static_cast<int8_t>(col)
                    });
                }
    
                const auto tex{GeneratePreviewTexture(shape)};
                Game->level.shapes.emplace_back(shape, tex);
            }
    
            // Spells
            file >> tag >> count;
            for (size_t i{0}; i < count; ++i)
            {
                int s{};
                file >> s;
                Game->level.spells.push_back(static_cast<Spell>(s));
            }
    
            std::println("Level lastet: saves/{}", filename);
        }

    void RemoveSpellAtIndex(const size_t index)
    {
        if (auto& spells{Game->level.spells}; index < spells.size())
        {
            spells.erase(spells.begin() + static_cast<int>(index));
        }
    }

    void Init()
    {
#ifndef _DEBUG
        SetTraceLogLevel(LOG_NONE); // Disable raylib trace log messages
#endif
        InitWindow(static_cast<int>(WindowWidth), static_cast<int>(WindowHeight), "");
        SetTargetFPS(TargetFps);
        InitAudioDevice();

        Game = std::make_unique<GameMemory>();
        Game->hexagon = LoadTexture("assets/textures/flathex.png");

        // Game Camera
        constexpr auto gamePixelHeight{180.f};
        Game->cameraGame.offset = UI::GameCameraStartPosition;
        Game->cameraGame.zoom = static_cast<float>(GetScreenHeight()) / gamePixelHeight;
        // UI Camera
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

    bool MousePositionInGameScreen()
    {
        const auto [x, y]{GetMousePosition()};
        return x < UI::GameWindowWidth && y < UI::GameWindowHeight;
    }

    void HandleInput()
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (Game->messageBoxState != MessageBoxState::none)
            {
                return;
            }

            if (!MousePositionInGameScreen())
            {
                return;
            }

            switch (Game->currentMode)
            {
            case Mode::game: break;
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
        if (IsMouseButtonDown(KEY_S))
        {
            
        }
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
            ClearBackground(BLACK);
            RenderMap(Game->level.tileMap);
            break;
        case Mode::editorNormal:
            ClearBackground(DARKBLUE);
            RenderMap(Game->level.tileMap);
            break;
        case Mode::editorAddShape:
            ClearBackground(BLUE);
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
            constexpr auto fpsTextSize{16};
            textPositionY += fpsTextSize;
        }
        // Shapes
        for (auto i{0}; i < Game->level.shapes.size(); ++i)
        {
            const auto& tex{Game->level.shapes.at(i).second};
            const Rectangle source = {
                .x = 0,
                .y = 0,
                .width = static_cast<float>(tex.texture.width),
                .height = -static_cast<float>(tex.texture.height)
            };
            constexpr auto scaleFactor{0.9f};
            constexpr auto indentValue{UI::LeftSideBar.width * (1.0f - scaleFactor) * 0.5f};
            float offsetY{indentValue};
            if (Game->currentMode != Mode::game)
            {
                offsetY = UI::EDITOR_SaveShapeButton.height + UI::EDITOR_AddShapeButton.height;
            }
            const Rectangle dest = {
                .x = UI::LeftSideBar.x + indentValue,
                .y = UI::LeftSideBar.y + offsetY + static_cast<float>(i) * (UI::LeftSideBar.width - indentValue),
                .width = UI::LeftSideBar.width * scaleFactor,
                .height = UI::LeftSideBar.width * scaleFactor,
            };

            if (GuiButton(dest, "") > 0)
            {
                switch (Game->currentMode)
                {
                case Mode::game:
                    {
                        if (const auto selectedShape{static_cast<int>(i)}; Game->currentShape == selectedShape)
                        {
                            Game->currentShape = {};
                        }
                        else
                        {
                            Game->currentShape = selectedShape;
                        }
                    }
                    break;
                case Mode::editorNormal:
                    {
                        Game->currentShape = static_cast<int>(i);
                        Game->messageBoxState = MessageBoxState::deleteShape;
                    }
                    break;
                case Mode::editorAddShape: break;
                }
            }

            DrawTexturePro(tex.texture, source, dest, Vector2{.x = 0, .y = 0}, 0.0f, WHITE);
        }

        // Spells
        for (auto i{0}; i < Game->level.spells.size(); ++i)
        {
            const auto spell{Game->level.spells.at(i)};
            constexpr auto scaleFactor{0.9f};
            constexpr auto indentValue{UI::BottomSideBar.height * (1.0f - scaleFactor) * 0.5f};

            const Rectangle dest = {
                .x = UI::BottomSideBar.x + indentValue + static_cast<float>(i) * (UI::BottomSideBar.height -
                    indentValue),
                .y = UI::BottomSideBar.y + indentValue,
                .width = UI::BottomSideBar.height * scaleFactor,
                .height = UI::BottomSideBar.height * scaleFactor,
            };

            if (GuiButton(dest, "") > 0)
            {
                switch (Game->currentMode)
                {
                case Mode::game:
                    {
                        if (const auto selectedSpell{static_cast<int>(i)}; Game->currentSpell == selectedSpell)
                        {
                            Game->currentSpell = {};
                        }
                        else
                        {
                            Game->currentSpell = selectedSpell;
                        }
                    }
                    break;
                case Mode::editorNormal:
                    {
                        Game->currentSpell = static_cast<int>(i);
                        Game->messageBoxState = MessageBoxState::deleteSpell;
                    }
                    break;
                case Mode::editorAddShape: break;
                }
            }

            DrawRectangleRec(dest, ToColor(spell));
        }

        if (Game->messageBoxState == MessageBoxState::deleteShape)
        {
            const auto result{
                GuiMessageBox(UI::MessageBox,
                              "",
                              std::format("Do you want to delete the current shape?").c_str(),
                              "YES;NO")
            };
            if (result == 1)
            {
                // DELETE
                if (Game->currentShape.has_value())
                {
                    RemoveShapeAtIndex(Game->currentShape.value());
                }

                Game->messageBoxState = MessageBoxState::none;
            }
            if (result == 2)
            {
                Game->messageBoxState = MessageBoxState::none;
            }
        }

        if (Game->messageBoxState == MessageBoxState::deleteSpell)
        {
            const auto result{
                GuiMessageBox(UI::MessageBox,
                              "",
                              std::format("Do you want to delete the current spell?").c_str(),
                              "YES;NO")
            };
            if (result == 1)
            {
                // DELETE
                if (Game->currentSpell.has_value())
                {
                    RemoveSpellAtIndex(Game->currentSpell.value());
                }

                Game->messageBoxState = MessageBoxState::none;
            }
            if (result == 2)
            {
                Game->messageBoxState = MessageBoxState::none;
            }
        }

        switch (Game->currentMode)
        {
        case Mode::game:
            {
                if (const auto currentShape{Game->currentShape}; currentShape.has_value())
                {
                    if (const auto index = currentShape.value(); index < Game->level.shapes.size())
                    {
                        const auto& tex{Game->level.shapes.at(index).second};
                        const Rectangle source = {
                            .x = 0,
                            .y = 0,
                            .width = static_cast<float>(tex.texture.width),
                            .height = -static_cast<float>(tex.texture.height)
                        };
                        constexpr auto scaleFactor{0.9f};
                        constexpr auto indentValue{UI::LeftSideBar.width * (1.0f - scaleFactor) * 0.5f};
                        constexpr Rectangle dest = {
                            .x = UI::MergeWindow.x + indentValue,
                            .y = UI::MergeWindow.y + indentValue,
                            .width = UI::MergeWindow.width * scaleFactor,
                            .height = UI::MergeWindow.height * scaleFactor,
                        };


                        const Color mergeColor{
                            Game->currentSpell.has_value()
                                ? ToColor(Game->level.spells.at(Game->currentSpell.value()))
                                : WHITE
                        };
                        DrawTexturePro(tex.texture, source, dest, Vector2{.x = 0, .y = 0}, 0.0f, mergeColor);
                    }
                }
            }
            break;
        case Mode::editorNormal:
            {
                if (GuiButton(UI::EDITOR_AddShapeButton, "Add Shape") > 0)
                {
                    Game->level.tempShape.Init();
                    Game->currentMode = Mode::editorAddShape;
                }
                if (GuiButton(UI::EDITOR_SaveLevelButton, "Save Level") > 0)
                {
                    SaveLevel("level1.txt");
                }
                if (GuiButton(UI::EDITOR_LoadLevelButton, "Load Level") > 0)
                {
                    LoadLevel("level1.txt");
                }

                for (const auto index : std::views::iota(0, static_cast<int>(Spell::count)))
                {
                    auto rect{UI::EDITOR_AddSpell};
                    rect.x += static_cast<float>(index) * UI::GenericButtonWidth;
                    if (const auto spell{static_cast<Spell>(index)};
                        GuiButton(rect, ToString(spell)) > 0)
                    {
                        AddSpell(spell);
                    }
                }
            }
            break;
        case Mode::editorAddShape:
            {
                if (GuiButton(UI::EDITOR_AddShapeButton, "Cancel") > 0)
                {
                    Game->currentMode = Mode::editorNormal;
                }
                if (GuiButton(UI::EDITOR_SaveShapeButton, "Save") > 0)
                {
                    Game->messageBoxState = MessageBoxState::saveShape;
                }
                if (Game->messageBoxState == MessageBoxState::saveShape)
                {
                    const auto result{
                        GuiMessageBox(UI::MessageBox,
                                      "Save",
                                      "Do you want to save the current shape?",
                                      "YES;NO")
                    };
                    if (result == 1) //YES
                    {
                        AddShape();
                        Game->messageBoxState = MessageBoxState::none;
                        Game->currentMode = Mode::editorNormal;
                    }
                    if (result == 2) //NO
                    {
                        Game->messageBoxState = MessageBoxState::none;
                    }
                    if (result > 0)
                    {
                        std::println("result, {}", result);
                    }
                }
            }
            break;
        }
        DrawText(std::format("Mode: {}", ToString(Game->currentMode)).c_str(),
                 0, textPositionY, 16, GREEN);

        EndMode2D();
    }

    void RenderFrame()
    {
        BeginDrawing();
        RenderGameScreen();
        RenderUI();
        EndDrawing();
    }

    //main game loop
    void Run()
    {
        assert(Game);
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
