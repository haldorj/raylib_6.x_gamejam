#pragma once

constexpr auto Offset{10};
constexpr auto Width{Offset * 2 + 1};
constexpr auto Height{Offset * 2 + 1};
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

    static constexpr auto iterator{std::views::iota(-Offset, Offset)};
    
private:
    static bool ValidIndex(int row, int col);
    
    std::array<MapTile, MapSize> m_map = {};
};
