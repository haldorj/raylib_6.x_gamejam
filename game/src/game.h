#pragma once
#include "tilemap.h"

constexpr auto WindowWidth{720.0f};
constexpr auto WindowHeight{720.0f};

constexpr auto TargetFps{120};

constexpr auto MinCameraZoom{2.0f};
constexpr auto MaxCameraZoom{8.0f};

constexpr auto HexagonSize{8.0};

enum class Mode : uint8_t
{
    game,
    editorNormal,
    editorAddShape,
};

enum class Spell : uint8_t
{
    fire,
    sickness,
    death,
    fear,
    obsession,

    count,
};

enum class MessageBoxState : uint8_t
{
    none,
    saveShape,
    deleteShape,
    deleteSpell,
};

// TODO: support saving and loading this struct
struct Level
{
    using shape = std::vector<MapTile>;
    // Level data
    MapTiles tileMap; // Main level
    MapTiles tempShape; // Shape editor level
    std::vector<std::pair<shape, RenderTexture>> shapes;
    std::vector<Spell> spells;
};

struct GameMemory
{
    Level level;

    Music music{};

    Camera2D cameraGame{};
    Camera2D cameraUI{};
    Vector2 mousePosition{};

    // TODO: make arrays for assets
    Sound fxButton{};
    Texture2D hexagon{};

    std::optional<int> currentShape;
    std::optional<int> currentSpell;

    MessageBoxState messageBoxState{MessageBoxState::none};
    Mode currentMode{Mode::game};
};

constexpr const char* ToString(const Mode mode)
{
    switch (mode)
    {
    case Mode::game: return "GAME";
    case Mode::editorNormal: return "EDITOR";
    case Mode::editorAddShape: return "ADD SHAPE";
    }
    return "UNKNOWN";
}

constexpr const char* ToString(const Spell spell)
{
    switch (spell)
    {
    case Spell::fire: return "fire";
    case Spell::sickness: return "sickness";
    case Spell::death: return "death";
    case Spell::fear: return "fear";
    case Spell::obsession: return "obsession";
    case Spell::count: break;
    }
    return "unknown";
}

constexpr Color ToColor(const Spell spell)
{
    switch (spell)
    {
    case Spell::fire: return RED;
    case Spell::sickness: return GREEN;
    case Spell::death: return GRAY;
    case Spell::fear: return DARKPURPLE;
    case Spell::obsession: return PINK;
    case Spell::count: break;
    }
    return WHITE;
}
