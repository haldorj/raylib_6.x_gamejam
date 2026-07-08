#pragma once

namespace UI
{
    // button /////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    enum class Alignment : uint8_t {
        left   = 0,
        top    = 0,
        centre = 1,
        middle = 1,
        right  = 2,
        bottom = 2
    };
    
    struct Button
    {
        std::function<void()> onPressed;
        Rectangle rect;
        std::string_view text;
        int fontSize{8};
        Alignment textAlignment{Alignment::centre};
    };
    
    void RenderButton(const Button& button);
    bool IsButtonHovered(const Button& button);
    
    
    // UI ELEMENTS ////////////////////////////////////////////////////////////////////////////////////////////////////
    
    // Left sidebar
    // Displays hex-shapes
    inline constexpr auto LeftSideBarHeight{0.8f};
    inline constexpr auto LeftSideBarWidth{1.0f - LeftSideBarHeight};
    inline constexpr Rectangle LeftSideBar{
        .x = static_cast<float>(GamePixelHeight) * LeftSideBarHeight,
        .y = 0.0f,
        .width = static_cast<float>(GamePixelHeight) * LeftSideBarWidth,
        .height = static_cast<float>(GamePixelHeight)
    };

    
    // Bottom sidebar
    // Displays elements
    inline constexpr auto BottomSideBarWidth{0.8f};
    inline constexpr auto BottomSideBarHeight{1.0f - BottomSideBarWidth};
    inline constexpr Rectangle BottomSideBar{
        .x = 0.0f,
        .y = static_cast<float>(GamePixelHeight) * BottomSideBarWidth,
        .width = static_cast<float>(GamePixelHeight),
        .height = static_cast<float>(GamePixelHeight) * BottomSideBarHeight
    };
    
    inline constexpr auto CameraStartPositionRelativeToUI = Vector2{
        .x = (WindowWidth * LeftSideBarHeight)/2.0f,
        .y = (WindowHeight * LeftSideBarHeight)/2.0f,
    };

    // MergeWindow
    // Displays Merge window
    inline constexpr auto MergeWindowWidth{0.22f};
    inline constexpr auto MergeWindowOffset{1.0f - MergeWindowWidth};

    inline constexpr Rectangle MergeWindow{
        .x = static_cast<float>(GamePixelHeight) * MergeWindowOffset,
        .y = static_cast<float>(GamePixelHeight) * MergeWindowOffset,
        .width = static_cast<float>(GamePixelHeight) * MergeWindowWidth,
        .height = static_cast<float>(GamePixelHeight) * MergeWindowWidth
    };
    
    // Add Button (Editor mode)
    inline constexpr Rectangle EDITOR_AddShapeButton{
        .x = LeftSideBar.x,
        .y = LeftSideBar.y,
        .width = LeftSideBar.width,
        .height = 16.f,
    };
}
