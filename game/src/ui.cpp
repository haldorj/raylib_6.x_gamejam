#include "pch.h"
#include "ui.h"

namespace UI
{
    void RenderButton(const Button& button)
    {
        if (IsButtonHovered(button))
        {
            DrawRectangleRec(button.rect, DARKGRAY);
        }
        else
        {
            DrawRectangleRec(button.rect, GRAY);
        }

        if (!button.text.empty())
        {
            DrawText(button.text.data(),
                     static_cast<int>(button.rect.x),
                     static_cast<int>(button.rect.y),
                     button.fontSize,
                     BLACK);
        }
    }

    auto IsButtonHovered(const Button& button) -> bool
    {
        const Vector2 mousePos{
            .x = GamePixelHeight * (GetMousePosition().x / static_cast<float>(GetScreenHeight())),
            .y = GamePixelHeight * (GetMousePosition().y / static_cast<float>(GetScreenHeight()))
        };
        return CheckCollisionPointRec(mousePos, button.rect);
    }
}
