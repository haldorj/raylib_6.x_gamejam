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
    // Measurements
    constexpr auto Size{8.0};
    [[maybe_unused]] constexpr auto Width{2.0 * Size};
    [[maybe_unused]] constexpr auto Height{std::numbers::sqrt3 * Size};

    // Distances
    [[maybe_unused]] constexpr auto Horizontal{Width * 0.75};
    [[maybe_unused]] constexpr auto Vertical{Height};
}

struct MapTile
{
    int row;
    int col;
    bool isValid;
};

struct MapTiles
{
    // Number of tiles in each direction from origin
    void Init()
    {
        for (const auto& row : iterator)
        {
            for (const auto& col : iterator)
            {
                auto& currenTile {map[row + offset][col + offset]};
                currenTile = {.row = row, .col = col};
                
                if (row == 0 && col == 0)
                {
                    currenTile.isValid = true;
                    continue;
                }
                currenTile.isValid = false;
            }
        }
    }

    void SetValid(const int row, const int col)
    {
        if (row < -offset || row > offset || col < -offset || col > offset)
        {
            return;
        }

        auto& currenTile {map[row + offset][col + offset]};
        currenTile.isValid = !currenTile.isValid;
    }

    static constexpr auto offset{10};
    static constexpr auto numRows{offset * 2 + 1};
    static constexpr auto numCols{offset * 2 + 1};

    static constexpr auto iterator{std::views::iota(-offset, offset)};

    MapTile map[numRows][numCols] = {};
};

struct GameState
{
    MapTiles tileMap;
};

static Vector2 HexToPixel(const int row, const int column)
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

static MapTile PixelToHex(const Vector2 point)
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

namespace
{
    constexpr auto WindowWidth{720};
    constexpr auto WindowHeight{720};
    constexpr auto TargetFps{120};
    constexpr auto GamePixelHeight{180};

    Camera2D Camera{};
    Texture2D Hexagon;
    Vector2 MousePosition{};

    GameState gameState;

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

        // Init map
        gameState = GameState{};
        gameState.tileMap.Init();
        
        assert(true);
    }

    void Shutdown()
    {
        CloseWindow();
    }

    void HandleInput()
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            const MapTile current{PixelToHex(MousePosition)};
            gameState.tileMap.SetValid(current.row, current.col);
        }
    }

    void UpdateGame()
    {
        MousePosition = Vector2Divide(Vector2Subtract(GetMousePosition(), Camera.offset),
                                      Vector2{.x = Camera.zoom, .y = Camera.zoom});
    }

    void DrawFrame()
    {
        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(Camera);

        const MapTile current{PixelToHex(MousePosition)};
        for (int row : MapTiles::iterator)
        {
            for (int col : MapTiles::iterator)
            {
                auto pos{HexToPixel(row, col)};
                const auto str{std::format("{}, {}", row, col)};

                pos.x -= static_cast<float>(hexagon::Size);
                pos.y -= static_cast<float>(hexagon::Size);

                auto color = WHITE;
                // origin
                if (row == 0 && col == 0)
                {
                    color = YELLOW;
                }

                if (row == current.row && col == current.col)
                {
                    color = current.isValid ? GREEN : GRAY;
                    DrawTextureEx(Hexagon, pos, 0.f, 1.0f, color);
                }

                if (gameState.tileMap.map[row + MapTiles::offset][col+ MapTiles::offset].isValid)
                {
                    DrawTextureEx(Hexagon, pos, 0.f, 1.0f, color);
                }
            }
        }
        DrawFPS(-GamePixelHeight / 2, -GamePixelHeight / 2);
        DrawText(std::format("Mouse X{}, Y{}", MousePosition.x, MousePosition.y).c_str(),
                 -GamePixelHeight / 2, 16 - GamePixelHeight / 2, 8, BLUE);

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
