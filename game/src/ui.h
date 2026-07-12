#pragma once
#include "game.h"

namespace UI
{
    // UI ELEMENTS ////////////////////////////////////////////////////////////////////////////////////////////////////

    constexpr Vector2 ScreenCenter{
        .x = WindowWidth * 0.5,
        .y = WindowHeight * 0.5,
    };

    // GenericButton
    inline constexpr auto GenericButtonHeight{24.0f};
    inline constexpr auto GenericButtonWidth{64.0f};
    // MessageBox
    inline constexpr auto MessageBoxWidth{250};
    inline constexpr auto MessageBoxHeight{100};
    inline constexpr Rectangle MessageBox{
        .x = ScreenCenter.x - (MessageBoxWidth * 0.5f),
        .y = ScreenCenter.y - (MessageBoxHeight * 0.5f),
        .width = MessageBoxWidth,
        .height = MessageBoxHeight
    };

    inline constexpr auto GameWindowWidth{580.0f};
    inline constexpr auto GameWindowHeight{580.0f};

    // Left sidebar
    // Displays hex-shapes
    inline constexpr Rectangle LeftSideBar{
        .x = GameWindowWidth,
        .y = 0.0f,
        .width = WindowWidth - GameWindowWidth,
        .height = WindowHeight
    };

    // Bottom sidebar
    // Displays elements
    inline constexpr Rectangle BottomSideBar{
        .x = 0.0f,
        .y = GameWindowHeight,
        .width = WindowHeight,
        .height = WindowWidth - GameWindowWidth
    };

    inline constexpr auto GameCameraStartPosition = Vector2{
        .x = GameWindowWidth / 2.0f,
        .y = GameWindowHeight / 2.0f,
    };

    // MergeWindow
    // Displays Merge window
    inline constexpr auto MergeWindowWidth{0.22f};
    inline constexpr auto MergeWindowOffset{1.0f - MergeWindowWidth};

    inline constexpr Rectangle MergeWindow{
        .x = static_cast<float>(WindowWidth) * MergeWindowOffset,
        .y = static_cast<float>(WindowHeight) * MergeWindowOffset,
        .width = static_cast<float>(WindowWidth) * MergeWindowWidth,
        .height = static_cast<float>(WindowHeight) * MergeWindowWidth
    };

    // Add Button (Editor mode)
    inline constexpr Rectangle EDITOR_AddShapeButton{
        .x = LeftSideBar.x,
        .y = LeftSideBar.y,
        .width = LeftSideBar.width,
        .height = GenericButtonHeight,
    };

    // Save shape
    inline constexpr Rectangle EDITOR_SaveShapeButton{
        .x = LeftSideBar.x,
        .y = LeftSideBar.y + GenericButtonHeight,
        .width = LeftSideBar.width,
        .height = GenericButtonHeight,
    };

    inline constexpr Rectangle EDITOR_LoadLevelButton{
        .x = LeftSideBar.x,
        .y = LeftSideBar.y + GenericButtonHeight * 3,
        .width = LeftSideBar.width,
        .height = GenericButtonHeight,
    };
    // AddSpellButton
    inline constexpr Rectangle EDITOR_AddSpell{
        .x = 0,
        .y = GameWindowHeight - GenericButtonHeight,
        .width = GenericButtonWidth,
        .height = GenericButtonHeight,
    };

    // Tekstboks for filnavn (over Save/Load-knappene, eller ved siden av)
    inline constexpr Rectangle EDITOR_SaveNameTextBox{
        .x = 0,
        .y = LeftSideBar.y + GenericButtonHeight * 2,
        .width = LeftSideBar.width,
        .height = GenericButtonHeight,
    };

    inline constexpr Rectangle EDITOR_SaveLevelButton{
        .x = 0,
        .y = LeftSideBar.y + GenericButtonHeight * 3,
        .width = LeftSideBar.width,
        .height = GenericButtonHeight,
    };

    // Start av listen over eksisterende lagringer
    inline constexpr Rectangle EDITOR_SaveListStart{
        .x = 0,
        .y = LeftSideBar.y + GenericButtonHeight * 4,
        .width = LeftSideBar.width,
        .height = GenericButtonHeight,
    };
}
