#include "pch.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>      // Emscripten library
#endif

#include <raylib.h>

namespace
{
    constexpr auto windowWidth{ 720 };
    constexpr auto windowHeight{ 720 };
    constexpr auto TargetFPS{ 60 };

    RenderTexture2D renderTexture{};
    Vector2 ballPosition{};
}

static void Init()
{
#if !defined(_DEBUG)
    SetTraceLogLevel(LOG_NONE); // Disable raylib trace log messages
#endif
    InitWindow(windowWidth, windowHeight, "raylib gamejam template");
    SetTargetFPS(TargetFPS);

    renderTexture = LoadRenderTexture(windowWidth, windowHeight);
    SetTextureFilter(renderTexture.texture, TEXTURE_FILTER_BILINEAR);

    ballPosition = { windowWidth / 2, windowHeight / 2 };
}

static void Shutdown()
{
    UnloadRenderTexture(renderTexture);
    CloseWindow();
}

static void  UpdateGame() 
{

}

static void HandleInput()
{
    if (IsKeyDown(KEY_RIGHT)) ballPosition.x += 2.0f;
    if (IsKeyDown(KEY_LEFT)) ballPosition.x -= 2.0f;
    if (IsKeyDown(KEY_UP)) ballPosition.y -= 2.0f;
    if (IsKeyDown(KEY_DOWN)) ballPosition.y += 2.0f;
}

static void DrawFrame()
{
    //frameCounter++;
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    // Render game screen to a texture, 
    // it could be useful for scaling or further shader postprocessing
    BeginTextureMode(renderTexture);
    ClearBackground(SKYBLUE);

    // TODO: Draw your game screen here
    DrawText("move the ball with arrow keys", 10, 10, 20, DARKGRAY);

    DrawCircleV(ballPosition, 50, MAROON);

    //
    EndTextureMode();

    // Render to screen (main framebuffer)
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Draw render texture to screen, scaled if required
    const auto source{ Rectangle{ 0, 0, (float)renderTexture.texture.width, -(float)renderTexture.texture.height } };
    const auto dest{ Rectangle{ 0, 0, (float)renderTexture.texture.width, (float)renderTexture.texture.height } };
    constexpr auto origin{ Vector2{ 0, 0 } };
    constexpr auto rotation{ 0.0f };
    DrawTexturePro(renderTexture.texture, source, dest, origin, rotation, WHITE);

    // TODO: Draw everything that requires to be drawn at this point, maybe UI?
    EndDrawing();
}

//main game loop
static void Run()
{
    HandleInput();
    UpdateGame();
    DrawFrame();
}

int main()
{
    Init();
#ifdef PLATFORM_WEB
    constexpr auto GameUpdateRateWEB{ 60 };
    emscripten_set_main_loop(Run, GameUpdateRateWEB, 1);
#else
    while (!WindowShouldClose())
    {
        Run();
    }
#endif
    Shutdown();
}
