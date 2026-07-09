#include "pch.h"
#include "ui.h"

namespace UI
{
    void RenderButton(const Button& button)
    {
        if (!IsButtonVisible(button))
        {
            return;
        }
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
            DrawText(button.text.c_str(),
                     static_cast<int>(button.rect.x),
                     static_cast<int>(button.rect.y),
                     button.fontSize,
                     BLACK);
        }
    }

    auto IsButtonHovered(const Button& button) -> bool
    {
        if (!IsButtonVisible(button))
        {
            return false;
        }
        const Vector2 mousePos{
            .x = GamePixelHeight * (GetMousePosition().x / static_cast<float>(GetScreenHeight())),
            .y = GamePixelHeight * (GetMousePosition().y / static_cast<float>(GetScreenHeight()))
        };
        return CheckCollisionPointRec(mousePos, button.rect);
    }

    bool IsButtonVisible(const Button& button)
    {
        if (const auto& visible = button.isVisible;
            visible)
        {
            return visible();
        }
        return true;
    }
}
