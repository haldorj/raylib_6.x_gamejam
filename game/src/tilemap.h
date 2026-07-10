#pragma once

constexpr auto MaxTilesFromCenter{10}; // Number of tiles in each direction from origin
constexpr auto Width{MaxTilesFromCenter * 2 + 1}; // +1 for center tile
constexpr auto Height{MaxTilesFromCenter * 2 + 1}; // +1 for center tile
constexpr auto MapSize{static_cast<size_t>(Width * Height * 2)};

enum Entity : uint8_t
{
    none,
};

struct MapTile
{
    int8_t row;
    int8_t col;
    bool isValid;   // Valid: can be interacted with and rendered to the screen
    Entity entity;  // Current entity on this tile
};

class MapTiles
{
public:
    void Init();

    MapTile& At(int row, int col);
    void SetValid(int row, int col);
    bool IsEmpty();
    
    std::vector<MapTile> ValidTiles();

    static constexpr auto iterator{std::views::iota(-MaxTilesFromCenter, MaxTilesFromCenter)};
    
private:
    static bool ValidIndex(int row, int col);
    
    std::array<MapTile, MapSize> m_map = {};
};
