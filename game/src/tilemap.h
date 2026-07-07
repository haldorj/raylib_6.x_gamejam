#pragma once

constexpr auto MaxTilesFromCenter{10};
constexpr auto Width{MaxTilesFromCenter * 2 + 1}; // +1 for center tile
constexpr auto Height{MaxTilesFromCenter * 2 + 1}; // +1 for center tile
constexpr auto MapSize{static_cast<size_t>(Width * Height * 2)};

struct MapTile
{
    int row;
    int col;
    bool isValid;
};

class MapTiles
{
public:
    // Number of tiles in each direction from origin
    void Init();

    MapTile& At(int row, int col);
    void SetValid(int row, int col);

    static constexpr auto iterator{std::views::iota(-MaxTilesFromCenter, MaxTilesFromCenter)};
    
private:
    static bool ValidIndex(int row, int col);
    
    std::array<MapTile, MapSize> m_map = {};
};
