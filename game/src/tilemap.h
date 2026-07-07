#pragma once

constexpr int Offset{10};
constexpr int Width{Offset * 2 + 1};
constexpr int Height{Offset * 2 + 1};

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
    
    MapTile m_map[Width][Height] = {};
};
