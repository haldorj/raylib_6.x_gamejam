#include "pch.h"
#include "ui.h"

namespace UI
{
    auto IsButtonHovered(const Button& button) -> bool
    {
        const Vector2 mousePos{
            .x = GamePixelHeight * (GetMousePosition().x / static_cast<float>(GetScreenHeight())),
            .y = GamePixelHeight * (GetMousePosition().y / static_cast<float>(GetScreenHeight()))
        };
        return CheckCollisionPointRec(mousePos, button.rect);
    }
}
