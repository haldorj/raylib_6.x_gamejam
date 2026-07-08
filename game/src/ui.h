#pragma once

namespace UI
{
    // button /////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    struct Button
    {
        std::function<void()> onPressed;
        Rectangle rect;
        std::string_view text;
    };
    
    bool IsButtonHovered(const Button& button);
    
    // UI ELEMENTS ////////////////////////////////////////////////////////////////////////////////////////////////////
    
    inline constexpr auto LeftSideBarHeight{0.8f};
    inline constexpr auto LeftSideBarWidth{1.0f - LeftSideBarHeight};
    inline constexpr Rectangle LeftSideBar{
        .x = static_cast<float>(GamePixelHeight) * LeftSideBarHeight,
        .y = 0.0f,
        .width = static_cast<float>(GamePixelHeight) * LeftSideBarWidth,
        .height = static_cast<float>(GamePixelHeight)
    };

    inline constexpr auto BottomSideBarWidth{0.8f};
    inline constexpr auto BottomSideBarHeight{1.0f - BottomSideBarWidth};
    inline constexpr Rectangle BottomSideBar{
        .x = 0.0f,
        .y = static_cast<float>(GamePixelHeight) * BottomSideBarWidth,
        .width = static_cast<float>(GamePixelHeight),
        .height = static_cast<float>(GamePixelHeight) * BottomSideBarHeight
    };

    inline constexpr auto MergeBarWidth{0.22f};
    inline constexpr auto MergeBarOffset{1.0f - MergeBarWidth};
    
    inline constexpr auto CameraStartPositionRelativeToUI = Vector2{
        .x = (WindowWidth * LeftSideBarHeight)/2.0f,
        .y = (WindowHeight * LeftSideBarHeight)/2.0f,
    };

    inline constexpr Rectangle MergeWindow{
        .x = static_cast<float>(GamePixelHeight) * MergeBarOffset,
        .y = static_cast<float>(GamePixelHeight) * MergeBarOffset,
        .width = static_cast<float>(GamePixelHeight) * MergeBarWidth,
        .height = static_cast<float>(GamePixelHeight) * MergeBarWidth
    };
}
