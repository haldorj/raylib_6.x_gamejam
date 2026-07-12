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

    // Returns neighboring hex coordinates around a hex
    auto AxialDirectionVectors(const int row, const int col) -> std::vector<std::pair<int, int>>
    {
        const auto hex = [row, col](const int rowDelta, const int colDelta) -> std::pair<int, int>
        {
            return {row - rowDelta, col - colDelta};
        };

        return {hex(+1, 0), hex(+1, -1), hex(0, -1), hex(-1, 0), hex(-1, +1), hex(0, +1)};
    }

    auto CurrentEnemyCount() -> int
    {
        auto count{0};
        for (const auto row : Game->level.tileMap.iterator)
        {
            for (const auto col : Game->level.tileMap.iterator)
            {
                if (Game->level.tileMap.At(row, col).isValid &&
                    Game->level.tileMap.At(row, col).entity == enemy)
                {
                    count++;
                }
            }
        }
        return count;
    }

    [[nodiscard]] auto FindPathToFirstInvalidHexInDirection(const int row, const int col,
                                                            const std::pair<int, int> dir)
        -> std::vector<std::pair<int, int>>
    {
        std::vector<std::pair<int, int>> path;

        int currentRow{row};
        int currentCol{col};

        while (Game->level.tileMap.ValidIndex(currentRow, currentCol))
        {
            if (!Game->level.tileMap.At(currentRow, currentCol).isValid)
            {
                break;
            }

            const auto [dirX, dirY]{dir};
            currentRow += dirX;
            currentCol += dirY;

            path.emplace_back(currentRow, currentCol);
        }

        if (!path.empty())
        {
            path.erase(path.begin());
        }

        return path;
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
        case Mode::editorEntityMode:
            return x < UI::GameWindowWidth - UI::GenericButtonHeight &&
                y < UI::GameWindowHeight - UI::GenericButtonHeight;
        }
        return false;
    }


    void Shutdown()
    {
        CloseWindow();
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

    void SaveLevel(const std::string& filename)
    {
        std::filesystem::create_directories("saves");
        std::ofstream file("saves/" + filename + ".txt");
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
            file << static_cast<int>(t.row) << " "
                << static_cast<int>(t.col) << " "
                << static_cast<int>(t.entity) << "\n";
        }

        // Shapes (kun rådata, ikke teksturen)
        file << "SHAPES " << Game->level.shapes.size() << "\n";
        for (const auto& shape : Game->level.shapes | std::views::keys)
        {
            file << shape.size() << "\n";
            for (const auto& t : shape)
            {
                file << static_cast<int>(t.row) << " " << static_cast<int>(t.col) << "\n";
            }
        }

        // Spells
        file << "SPELLS " << Game->level.spells.size() << "\n";
        for (const auto& spell : Game->level.spells)
        {
            file << static_cast<int>(spell) << "\n";
        }

        std::println("Level lagret: saves/{}", filename);
    }

    void RefreshAvailableSaves()
    {
        Game->availableSaves.clear();
        std::filesystem::create_directories("saves");
        for (const auto& entry : std::filesystem::directory_iterator("saves"))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".txt")
            {
                Game->availableSaves.push_back(entry.path().stem().string()); // uten .txt
            }
        }
    }

    void LoadLevel(const std::string& filename)
    {
        std::ifstream file("saves/" + filename + ".txt");
        if (!file)
        {
            std::println("Fant ikke lagret fil: {}", filename);
            return;
        }

        // Rydd opp gammel state først
        for (const auto& tex : Game->level.shapes | std::views::values)
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
            int row{};
            int col{};
            int entity{};
            file >> row >> col >> entity;
            Game->level.tileMap.SetValid(row, col);
            Game->level.tileMap.SetEntity(static_cast<Entity>(entity), row, col);
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
                int row{};
                int col{};
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
        for (auto i{0uz}; i < count; ++i)
        {
            int s{};
            file >> s;
            Game->level.spells.push_back(static_cast<Spell>(s));
        }

        std::println("Level lastet: saves/{}", filename);
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
        Game->explosiveRad = LoadTexture("assets/textures/object/tnt.png");
        Game->explosiveDirUp = LoadTexture("assets/textures/object/tnt_up.png");
        Game->explosiveDirLeft = LoadTexture("assets/textures/object/tnt_left.png");
        Game->explosiveDirRight = LoadTexture("assets/textures/object/tnt_right.png");
        Game->enemy = LoadTexture("assets/textures/Characters/assassin.png");
        RefreshAvailableSaves();

        // Game Camera
        constexpr auto gamePixelHeight{180.f};
        Game->cameraGame.offset = UI::GameCameraStartPosition;
        Game->cameraGame.zoom = static_cast<float>(GetScreenHeight()) / gamePixelHeight;
        // UI Camera
        Game->cameraUI.zoom = 1.0f; // no scale;

        Game->music = LoadMusicStream("assets/Music/WaveyBlue.wav");
        SetMusicVolume(Game->music, MasterVolume * MusicVolume);
        Game->fxButton = LoadSound("assets/Sound effects/UI sounds/menu_blip.wav");
        Game->explosionMedium = LoadSound("assets/Sound effects/Object/explosion_medium.wav");
        Game->explosionLarge = LoadSound("assets/Sound effects/Object/explosion_large.wav");

        Game->renderTexture = LoadRenderTexture(static_cast<int>(WindowWidth), static_cast<int>(WindowHeight));
        // float timePlayed = 0.0f; // Time played normalized [0.0f..1.0f]
        constexpr float pan = 0.0f; // Default audio pan center [-1.0f..1.0f]
        SetMusicPan(Game->music, pan);
        // Init map
        Game->level.tileMap.Init();
        PlayMusicStream(Game->music);
        
        // LOAD FIRST LEVEL
        Game->currentLevelIndex = 0;
        LoadLevel(Game->availableSaves.at(Game->currentLevelIndex));

#ifdef PLATFORM_WEB
        Game->shader = LoadShader(nullptr, "assets/shaders/shader_web.fs");
#endif
#ifndef PLATFORM_WEB
        Game->shader = LoadShader(nullptr, "assets/shaders/shader.fs");
        Running = true;
#endif
    }

    auto CheckShapeCollisionWithMap(MapTiles& map) -> bool
    {
        const auto index{Game->currentShapeIdx};
        const auto mousePos{PixelToHex(Game->mousePosition)};
        if (!index.has_value())
        {
            return false;
        }
        for (const auto shape : Game->level.shapes.at(index.value()).first)
        {
            const auto adjustedRow{shape.row + mousePos.row};
            const auto adjustedCol{shape.col + mousePos.col};
            if (!map.ValidIndex(adjustedRow, adjustedCol))
            {
                return false;
            }
            if (!map.At(adjustedRow, adjustedCol).isValid)
            {
                return false;
            }
        }
        return true;
    }
    
    void ExplodeArea(MapTile& mapTile);
    void ExplodeBarrelRad(const int adjustedRow, const int adjustedCol);

    auto ExplodeArea(MapTile& mapTile) -> void
    {
        if (mapTile.entity == explosiveRad)
        {
            ExplodeBarrelRad(mapTile.row, mapTile.col);
        }
        if (mapTile.entity == enemy)
        {
            mapTile.entity = none;
        }
        if (mapTile.entity == enemy)
        {
            mapTile.entity = none;
        }
        if (mapTile.entity == enemy)
        {
            mapTile.entity = none;
        }
    };
    
    void ExplodeBarrelRad(const int adjustedRow, const int adjustedCol)
    {
        auto& map{Game->level.tileMap};
        // Explode in a radius around the entity
        PlaySound(Game->explosionMedium);
        map.At(adjustedRow, adjustedCol).entity = none;

        for (const auto [row, col] : AxialDirectionVectors(adjustedRow, adjustedCol))
        {
            ExplodeArea(map.At(row, col));
        }
    }
    
    auto PlaceCurrentShape() -> void
    {
        auto& map{Game->level.tileMap};
        if (!CheckShapeCollisionWithMap(map))
        {
            return;
        }

        const auto mousePos{PixelToHex(Game->mousePosition)};
        if (!Game->currentSpellIdx.has_value())
        {
            return;
        }

        for (const auto& shape : Game->level.shapes.at(Game->currentShapeIdx.value()).first)
        {
            const auto adjustedRow{shape.row + mousePos.row};
            const auto adjustedCol{shape.col + mousePos.col};
            if (!map.ValidIndex(adjustedRow, adjustedCol))
            {
                return;
            }

            if (!map.At(adjustedRow, adjustedCol).isValid)
            {
                return;
            }

            switch (Game->level.spells.at(Game->currentSpellIdx.value()))
            {
            case Spell::fire:
                {
                    if (map.At(adjustedRow, adjustedCol).entity == explosiveRad)
                    {
                        ExplodeBarrelRad(adjustedRow, adjustedCol);
                    }
                    if (map.At(adjustedRow, adjustedCol).entity == explosiveDirUp)
                    {
                        PlaySound(Game->explosionMedium);

                        auto path{FindPathToFirstInvalidHexInDirection(adjustedRow, adjustedCol, North)};
                        const auto& path2{FindPathToFirstInvalidHexInDirection(adjustedRow, adjustedCol, South)};

                        // combine 
                        path.reserve(path.size() + path.size());
                        path.insert(path.end(), path2.begin(), path2.end());

                        for (const auto [row, col] : path)
                        {
                            ExplodeArea(map.At(row, col));
                        }
                    }
                    if (map.At(adjustedRow, adjustedCol).entity == explosiveDirLeft)
                    {
                        PlaySound(Game->explosionMedium);

                        auto path{FindPathToFirstInvalidHexInDirection(adjustedRow, adjustedCol, NorthWest)};
                        const auto& path2{FindPathToFirstInvalidHexInDirection(adjustedRow, adjustedCol, SouthEast)};

                        // combine 
                        path.reserve(path.size() + path.size());
                        path.insert(path.end(), path2.begin(), path2.end());

                        for (const auto [row, col] : path)
                        {
                            ExplodeArea(map.At(row, col));
                        }
                    }
                    if (map.At(adjustedRow, adjustedCol).entity == explosiveDirRight)
                    {
                        PlaySound(Game->explosionMedium);

                        auto path{FindPathToFirstInvalidHexInDirection(adjustedRow, adjustedCol, NorthEast)};
                        const auto& path2{FindPathToFirstInvalidHexInDirection(adjustedRow, adjustedCol, SouthWest)};

                        // combine 
                        path.reserve(path.size() + path.size());
                        path.insert(path.end(), path2.begin(), path2.end());

                        for (const auto [row, col] : path)
                        {
                            ExplodeArea(map.At(row, col));
                        }
                    }
                    if (map.At(adjustedRow, adjustedCol).entity == enemy)
                    {
                        ExplodeArea(map.At(adjustedRow, adjustedCol));
                    }
                }
                break;
            case Spell::sickness:
                {
                }
                break;
            case Spell::death:
                {
                }
                break;
            case Spell::fear:
                {
                }
                break;
            case Spell::obsession:
                {
                }
                break;
            case Spell::count:
                break;
            }
        }
    }

    auto HandleClickGameScreen() -> auto
    {
        switch (Game->mode)
        {
        case Mode::game:
            {
                PlaceCurrentShape();
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
        case Mode::editorEntityMode:
            {
                const MapTile current{PixelToHex(Game->mousePosition)};
                Game->level.tileMap.SetEntity(Game->currentEntity, current.row, current.col);
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
        default: break;
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
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            const auto delta{GetMouseDelta()};
            Game->cameraGame.offset = Vector2Add(Game->cameraGame.offset, delta);
        }
        if (IsMouseButtonDown(KEY_S))
        {
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
                if (!map.ValidIndex(mousePos.row, mousePos.col))
                {
                    return;
                }
                const auto color = map.At(mousePos.row, mousePos.col).isValid ? RED : GREEN;
                DrawTextureEx(Game->hexagon, hexAtMousePos, 0.f, 1.0f, color);
            }
        };

        switch (mode)
        {
        case Mode::game:
            {
                if (Game->currentShapeIdx.has_value())
                {
                    const auto index{static_cast<size_t>(Game->currentShapeIdx.value())};
                    for (auto const& shape : Game->level.shapes.at(index).first)
                    {
                        const auto hex{HexToPixel(shape.row, shape.col)};
                        const auto color{CheckShapeCollisionWithMap(map) ? GREEN : RED};
                        DrawTextureEx(Game->hexagon, Vector2Add(hex, hexAtMousePos), 0.f, 1.0f, color);
                    }
                }
                else
                {
                    renderSingleHex();
                }
            }
            break;
        case Mode::editorNormal:
        case Mode::editorAddShape:
        case Mode::editorEntityMode:
            {
                renderSingleHex();
            }
            break;
        }
    }

    auto RenderEntity(const Entity entity, const Vector2 position)
    {
        if (entity == none)
        {
            return;
        }

        // Add some offset in Y direction to create a feeling of perspective.
        const auto adjustedPosition{Vector2Add(position, {.x = 0.0f, .y = -4.0f})};

        switch (entity)
        {
        case none: return;
        case explosiveRad:
            DrawTextureEx(Game->explosiveRad, adjustedPosition, 0.f, 1.0f, WHITE);
            break;
        case explosiveDirUp:
            DrawTextureEx(Game->explosiveDirUp, adjustedPosition, 0.f, 1.0f, WHITE);
            break;
        case explosiveDirLeft:
            DrawTextureEx(Game->explosiveDirLeft, adjustedPosition, 0.f, 1.0f, WHITE);
            break;
        case explosiveDirRight:
            DrawTextureEx(Game->explosiveDirRight, adjustedPosition, 0.f, 1.0f, WHITE);
            break;
        case enemy:
            {
                const auto& tex{Game->enemy};
                const auto scale{16.0f / static_cast<float>(tex.height)};
                DrawTextureEx(Game->enemy, adjustedPosition, 0.f, scale, WHITE);
            }
            break;


        case count:
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
                if (const auto hex{map.At(row, col)}; hex.isValid)
                {
                    DrawTextureEx(Game->hexagon, pos, 0.f, 1.0f, WHITE);
                    RenderEntity(hex.entity, pos);
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
            {
                ClearBackground(BLACK);
                RenderMap(Game->level.tileMap);
                RenderCurrentShape(Game->level.tileMap, Mode::game);
            }
            break;
        case Mode::editorNormal:
            {
                ClearBackground(DARKBLUE);
                RenderMap(Game->level.tileMap);
                RenderCurrentShape(Game->level.tileMap, Mode::editorNormal);
            }
            break;
        case Mode::editorAddShape:
            {
                ClearBackground(BLUE);
                RenderMap(Game->level.tempShape);
                RenderCurrentShape(Game->level.tempShape, Mode::editorAddShape);
            }
            break;
        case Mode::editorEntityMode:
            {
                ClearBackground(DARKBLUE);
                RenderMap(Game->level.tileMap);
                RenderCurrentShape(Game->level.tileMap, Mode::editorEntityMode);
            }
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
                if (Game->currentShapeIdx.has_value())
                {
                    RemoveShapeAtIndex(Game->currentShapeIdx.value());
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
                if (Game->currentSpellIdx.has_value())
                {
                    RemoveSpellAtIndex(Game->currentSpellIdx.value());
                }

                Game->messageBoxState = MessageBoxState::none;
            }
            if (result == 2)
            {
                Game->messageBoxState = MessageBoxState::none;
            }
        }
        
        if (Game->messageBoxState == MessageBoxState::levelWin)
        {
            const auto result{
                GuiMessageBox(UI::MessageBox,
                              "Win",
                              "You won!",
                              "Next Level")
            };
            if (result == 1) //YES
            {
                // NextLevel
                Game->messageBoxState = MessageBoxState::none;
                Game->LevelCounter++;
                LoadLevel(Game->availableSaves.at(Game->currentLevelIndex+Game->LevelCounter));
            }
        }
    }

    auto RenderMergeWindow() -> void
    {
        const auto currentShape{Game->currentShapeIdx};
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

            Color mergeColor;
            if (Game->currentSpellIdx.has_value())
            {
                mergeColor = ToColor(Game->level.spells.at(Game->currentSpellIdx.value()));
            }
            else
            {
                mergeColor = WHITE;
            }
            DrawTexturePro(tex.texture, source, dest, Vector2{.x = 0, .y = 0}, 0.0f, mergeColor);
        }
    }

    auto DrawShapeSideBar() -> void
    {
        // Shapes
        for (auto i{0}; i < Game->level.shapes.size(); ++i)
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
                        if (const auto selectedShape{static_cast<int>(i)}; Game->currentShapeIdx == selectedShape)
                        {
                            Game->currentShapeIdx = {};
                        }
                        else
                        {
                            Game->currentShapeIdx = selectedShape;
                        }
                    }
                    break;
                case Mode::editorNormal:
                    {
                        Game->currentShapeIdx = static_cast<int>(i);
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
        for (auto i{0}; i < Game->level.spells.size(); ++i)
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
                        if (const auto selectedSpell{i}; Game->currentSpellIdx == selectedSpell)
                        {
                            Game->currentSpellIdx = {};
                        }
                        else
                        {
                            Game->currentSpellIdx = selectedSpell;
                        }
                    }
                    break;
                case Mode::editorNormal:
                    {
                        Game->currentSpellIdx = i;
                        Game->messageBoxState = MessageBoxState::deleteSpell;
                    }
                    break;
                case Mode::editorAddShape:
                case Mode::editorEntityMode: break;
                }
            }

            // Render texture on top of button
            DrawRectangleRec(dest, ToColor(spell));
        }
    }

    void DrawEntitySideBar()
    {
        // Spells
        constexpr auto entities{std::views::iota(0, static_cast<int>(Entity::count))};
        for (const auto entity : entities)
        {
            // Calculate button/texture position
            constexpr auto padding{5.0f};
            constexpr auto buttonWidth{32.0f};

            const Rectangle dest = {
                .x = UI::BottomSideBar.x + padding + static_cast<float>(entity) * (buttonWidth + padding),
                .y = UI::BottomSideBar.y + padding,
                .width = buttonWidth,
                .height = buttonWidth,
            };

            // Button functionality
            if (GuiButton(dest, "") > 0)
            {
                Game->currentEntity = static_cast<Entity>(entity);
            }

            // Render texture on top of button
            RenderEntity(static_cast<Entity>(entity), {.x = dest.x + 8.0f, .y = dest.y + 12.0f});
        }
    }
}

void RenderSaveOptions()
{
    if (GuiButton(UI::EDITOR_AddShapeButton, "Add Shape") > 0)
    {
        Game->level.tempShape.Init();
        Game->mode = Mode::editorAddShape;
    }

    // --- NYTT: erstatter de to gamle if-blokkene ---
    if (GuiTextBox(UI::EDITOR_SaveNameTextBox, Game->saveNameBuffer.data(),
                   static_cast<int>(Game->saveNameBuffer.size()), Game->saveNameEditMode) > 0)
    {
        Game->saveNameEditMode = !Game->saveNameEditMode;
    }

    if (GuiButton(UI::EDITOR_SaveLevelButton, "Save Level") > 0)
    {
        SaveLevel(Game->saveNameBuffer.data());
        RefreshAvailableSaves();
    }

    for (size_t i{0}; i < Game->availableSaves.size(); ++i)
    {
        auto rect{UI::EDITOR_SaveListStart};
        rect.y += static_cast<float>(i) * UI::GenericButtonHeight;

        if (GuiButton(rect, Game->availableSaves.at(i).c_str()) > 0)
        {
            LoadLevel(Game->availableSaves.at(i));
        }
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

    // --- SLUTT NYTT ---
    switch (Game->mode)
    {
    case Mode::game:
        {
            RenderMergeWindow();
            DrawSpellSideBar();
        }
        break;
    case Mode::editorNormal:
        {
            DrawSpellSideBar();
            RenderSaveOptions();

            if (GuiButton(UI::EDITOR_AddShapeButton, "Add Shape") > 0)
            {
                Game->level.tempShape.Init();
                Game->mode = Mode::editorAddShape;
            }
            if (GuiButton(UI::EDITOR_SaveShapeButton, "Add Entities") > 0)
            {
                Game->mode = Mode::editorEntityMode;
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
            DrawSpellSideBar();

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
    case Mode::editorEntityMode:
        {
            DrawEntitySideBar();
            if (GuiButton(UI::EDITOR_AddShapeButton, "Cancel") > 0)
            {
                Game->mode = Mode::editorNormal;
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
    
    // Win condition
    Game->numOfEnemies = CurrentEnemyCount();
    if (Game->numOfEnemies <= 0 && Game->mode == Mode::game)
    {
        Game->messageBoxState = MessageBoxState::levelWin;
    }
    
    // Handle rendering
    //BeginDrawing();
    BeginTextureMode(Game->renderTexture);
    RenderGameScreen();
    RenderUI();
    EndTextureMode();
    //EndDrawing();

    // Post-processing
    BeginDrawing();
    ClearBackground(BLACK);
    BeginShaderMode(Game->shader);
    const auto dest{
        Rectangle{
            .x = 0,
            .y = 0,
            .width = static_cast<float>(Game->renderTexture.texture.width),
            .height = static_cast<float>(-Game->renderTexture.texture.height)
        }
    };
    DrawTextureRec(Game->renderTexture.texture,
                   dest,
                   Vector2{.x = 0, .y = 0},
                   WHITE);
    EndShaderMode();
    EndDrawing();
}

//main game loop
void Run()
{
    assert(Game);
    HandleInput();
    UpdateAndRender();
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
