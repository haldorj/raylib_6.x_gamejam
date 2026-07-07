#include "pch.h"

#include "tilemap.h"

void MapTiles::Init()
{
    for (const auto row : iterator)
    {
        for (const auto col : iterator)
        {
            auto& currenTile {m_map[row + Offset][col + Offset]};
            currenTile = {.row = row, .col = col};
                
            if (row == 0 && col == 0)
            {
                currenTile.isValid = true;
                continue;
            }
            currenTile.isValid = false;
        }
    }
}

MapTile& MapTiles::At(const int row, const int col)
{
    assert(ValidIndex(row, col));
    return m_map[row + Offset][col + Offset];
}

void MapTiles::SetValid(const int row, const int col)
{
    if (!ValidIndex(row, col))
    {
        return;
    }

    auto& currenTile {m_map[row + Offset][col + Offset]};
    currenTile.isValid = !currenTile.isValid;
}

bool MapTiles::ValidIndex(const int row, const int col)
{
    return (row >= -Offset && row <= Offset && col >= -Offset && col <= Offset);
}
