#include "pch.h"

#include "ui.h"
#include "tilemap.h"
#include "game.h"

namespace
{
    std::unique_ptr<GameMemory> Game;
#ifndef PLATFORM_WEB
    bool Running;
#endif
    auto MasterVolume{0.5f};
    auto MusicVolume{1.0f};

    auto HexToPixel(const int row, const int column) -> Vector2
    {
        const auto x{3.0 / 2 * static_cast<double>(column)};
        const auto y{std::numbers::sqrt3 / 2.0 * column + std::numbers::sqrt3 * row};
        const auto result = Vector2{
            .x = static_cast<float>(x * HexagonSize),
            .y = static_cast<float>(y * HexagonSize)
        };
        return result;
    }

    auto PixelToHex(const Vector2 point) -> MapTile
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

    auto MousePositionInGameScreen() -> bool
    {
        const auto [x, y]{GetMousePosition()};
        switch (Game->mode)
        {
        case Mode::game:
            return x < UI::GameWindowWidth && y < UI::GameWindowHeight;
        case Mode::editorNormal:
        case Mode::editorAddShape:
            return x < UI::GameWindowWidth - UI::GenericButtonHeight &&
                y < UI::GameWindowHeight - UI::GenericButtonHeight;
        }
        return false;
    }

    auto GeneratePreviewTexture(const std::span<MapTile> shape) -> RenderTexture
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

    auto AddShape() -> void
    {
        if (!Game->level.tempShape.IsEmpty())
        {
            auto newShape{Game->level.tempShape.ValidTiles()};
            const auto previewTexture{GeneratePreviewTexture(newShape)};
            Game->level.shapes.emplace_back(newShape, previewTexture);
        }
    }

    auto RemoveShapeAtIndex(const size_t index) -> void
    {
        if (auto& shapes{Game->level.shapes}; index < shapes.size())
        {
            UnloadRenderTexture(shapes.at(index).second);
            shapes.erase(shapes.begin() + static_cast<int>(index));
        }
    }

    auto AddSpell(const Spell spell) -> void
    {
        Game->level.spells.push_back(spell);
    }

    auto RemoveSpellAtIndex(const size_t index) -> void
    {
        if (auto& spells{Game->level.spells}; index < spells.size())
        {
            spells.erase(spells.begin() + static_cast<int>(index));
        }
    }

    auto Init() -> void
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
#ifdef _DEBUG
        SetMusicVolume(Game->music, 0.0f);
#elif
        SetMusicVolume(Game->music, MasterVolume * MusicVolume);
#endif
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

    // auto PlaceCurrentShape() -> void
    // {
    //     const MapTile current{PixelToHex(Game->mousePosition)};
    //     if (Game->currentShape.has_value())
    //     {
    //         const auto shapeIndex{static_cast<size_t>(Game->currentShape.value())};
    //         const auto& currentShape{Game->level.shapes.at(shapeIndex)};
    //     }
    // }

    auto HandleClickGameScreen() -> auto
    {
        switch (Game->mode)
        {
        case Mode::game:
            {
                //PlaceCurrentShape();
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

    auto SwapGameAndEditorMode() -> void
    {
        switch (Game->mode)
        {
        case Mode::game:
            Game->mode = Mode::editorNormal;
            break;
        case Mode::editorNormal:
            Game->mode = Mode::game;
            break;
        default: ;
        }
    }

    auto HandleInput() -> void
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
            HandleClickGameScreen();
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            // Panning
            const auto delta{GetMouseDelta()};
            Game->cameraGame.offset = Vector2Add(Game->cameraGame.offset, delta);
        }
        if (IsKeyPressed(KEY_F2))
        {
            SwapGameAndEditorMode();
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

    auto RenderCurrentShape(MapTiles& map, const Mode mode)
    {
        const auto mousePos{PixelToHex(Game->mousePosition)};
        auto hexAtMousePos{HexToPixel(mousePos.row, mousePos.col)};
        hexAtMousePos.x -= static_cast<float>(HexagonSize);
        hexAtMousePos.y -= static_cast<float>(HexagonSize);
        
        auto renderSingleHex{
            [&map, hexAtMousePos, mousePos]
            {
                const auto color = map.At(mousePos.row, mousePos.col).isValid ? RED : GREEN;
                DrawTextureEx(Game->hexagon, hexAtMousePos, 0.f, 1.0f, color);
            }
        };

        switch (mode)
        {
        case Mode::game:
            {
                if (Game->currentShape.has_value())
                {
                    const auto index{static_cast<size_t>(Game->currentShape.value())};
                    for (auto const& shape : Game->level.shapes.at(index).first)
                    {
                        const auto hex{HexToPixel(shape.row, shape.col)};
                        DrawTextureEx(Game->hexagon, Vector2Add(hex, hexAtMousePos), 0.f, 1.0f, GREEN);
                    }
                }
                else
                {
                    renderSingleHex();
                }
            }
            break;
        case Mode::editorNormal:
            {
                renderSingleHex();
            }
            break;
        case Mode::editorAddShape:
            {
                renderSingleHex();
            }
            break;
        }
    }

    auto RenderMap(MapTiles& map) -> void
    {
        for (const int row : MapTiles::iterator)
        {
            for (const int col : MapTiles::iterator)
            {
                auto pos{HexToPixel(row, col)};
                pos.x -= static_cast<float>(HexagonSize);
                pos.y -= static_cast<float>(HexagonSize);

                if (map.At(row, col).isValid)
                {
                    DrawTextureEx(Game->hexagon, pos, 0.f, 1.0f, WHITE);
                }
            }
        }
    }

    auto RenderGameScreen() -> void
    {
        BeginMode2D(Game->cameraGame);
        switch (Game->mode)
        {
        case Mode::game:
            ClearBackground(BLACK);
            RenderMap(Game->level.tileMap);
            RenderCurrentShape(Game->level.tileMap, Mode::game);
            break;
        case Mode::editorNormal:
            ClearBackground(DARKBLUE);
            RenderMap(Game->level.tileMap);
            RenderCurrentShape(Game->level.tileMap, Mode::editorNormal);
            break;
        case Mode::editorAddShape:
            ClearBackground(BLUE);
            RenderMap(Game->level.tempShape);
            RenderCurrentShape(Game->level.tempShape, Mode::editorAddShape);
            break;
        }
        EndMode2D();
    }

    void ShowMessageBox()
    {
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
                Game->mode = Mode::editorNormal;
            }
            if (result == 2) //NO
            {
                Game->messageBoxState = MessageBoxState::none;
            }
            if (result > 0)
            {
                /*std::println("result, {}", result);*/
            }
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
    }

    auto RenderMergeWindow() -> void
    {
        const auto currentShape{Game->currentShape};
        if (!currentShape.has_value())
        {
            return;
        }

        if (const auto index = currentShape.value(); std::cmp_less(index, Game->level.shapes.size()))
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

    auto DrawShapeSideBar() -> void
    {
        // Shapes
        for (auto i{0uz}; i < Game->level.shapes.size(); ++i)
        {
            // Calculate button/texture position
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
            if (Game->mode != Mode::game)
            {
                offsetY = UI::EDITOR_SaveShapeButton.height + UI::EDITOR_AddShapeButton.height;
            }
            const Rectangle dest = {
                .x = UI::LeftSideBar.x + indentValue,
                .y = UI::LeftSideBar.y + offsetY + static_cast<float>(i) * (UI::LeftSideBar.width - indentValue),
                .width = UI::LeftSideBar.width * scaleFactor,
                .height = UI::LeftSideBar.width * scaleFactor,
            };

            // Button functionality
            if (GuiButton(dest, "") > 0)
            {
                switch (Game->mode)
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
            // Render texture on top of button
            DrawTexturePro(tex.texture, source, dest, Vector2{.x = 0, .y = 0}, 0.0f, WHITE);
        }
    }

    void DrawSpellSideBar()
    {
        // Spells
        for (auto i{0uz}; i < Game->level.spells.size(); ++i)
        {
            // Calculate button/texture position
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

            // Button funtionality
            if (GuiButton(dest, "") > 0)
            {
                switch (Game->mode)
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

            // Render texture on top of button
            DrawRectangleRec(dest, ToColor(spell));
        }
    }

    auto RenderUI() -> void
    {
        BeginMode2D(Game->cameraUI);

        DrawRectangleRec(UI::LeftSideBar, LIGHTGRAY);
        DrawRectangleRec(UI::BottomSideBar, LIGHTGRAY);
        DrawRectangleRec(UI::MergeWindow, DARKGRAY);

        auto textPositionY{0};
        if (Game->mode == Mode::editorNormal)
        {
            DrawFPS(0, 0);
            constexpr auto fpsTextSize{16};
            textPositionY += fpsTextSize;
        }

        DrawShapeSideBar();
        DrawSpellSideBar();

        switch (Game->mode)
        {
        case Mode::game:
            {
                RenderMergeWindow();
            }
            break;
        case Mode::editorNormal:
            {
                if (GuiButton(UI::EDITOR_AddShapeButton, "Add Shape") > 0)
                {
                    Game->level.tempShape.Init();
                    Game->mode = Mode::editorAddShape;
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
                    Game->mode = Mode::editorNormal;
                }
                if (GuiButton(UI::EDITOR_SaveShapeButton, "Save") > 0)
                {
                    Game->messageBoxState = MessageBoxState::saveShape;
                }
            }
            break;
        }
        DrawText(std::format("Mode: {}", ToString(Game->mode)).c_str(),
                 0, textPositionY, 16, GREEN);

        ShowMessageBox();

        EndMode2D();
    }

    void UpdateAndRender()
    {
        // Update game logic
        Game->mousePosition = Vector2Divide(Vector2Subtract(GetMousePosition(), Game->cameraGame.offset),
                                            Vector2{.x = Game->cameraGame.zoom, .y = Game->cameraGame.zoom});

        UpdateMusicStream(Game->music); // Update music buffer with new stream data

        // Handle rendering
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
        UpdateAndRender();
    }

    void Shutdown()
    {
        CloseWindow();
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
