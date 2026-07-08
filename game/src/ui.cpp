#include "pch.h"
#include "ui.h"

namespace UI
{
    auto IsButtonHovered(const Button& button) -> bool
    {
        const Vector2 mousePos = GetMousePosition();
        return CheckCollisionPointRec(mousePos, button.rect);
    }
}
