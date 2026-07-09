#include "pch.h"

#include "tilemap.h"

void MapTiles::Init()
{
    for (const auto row : iterator)
    {
        for (const auto col : iterator)
        {
            auto& currenTile {m_map.at(row + MaxTilesFromCenter + (col + MaxTilesFromCenter) * Width)};
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
    return m_map.at(row + MaxTilesFromCenter + (col + MaxTilesFromCenter) * Width);
}

void MapTiles::SetValid(const int row, const int col)
{
    if (!ValidIndex(row, col))
    {
        return;
    }

    auto& currenTile {At(row, col)};
    currenTile.isValid = !currenTile.isValid;
}

bool MapTiles::IsEmpty()
{
    for (const auto row : iterator)
    {
        for (const auto col : iterator)
        {
            if (At(row, col).isValid)
            {
                return false;
            }
        }
    }
    return true;
}

std::vector<MapTile> MapTiles::ValidTiles()
{
    std::vector<MapTile> result;
    result.reserve(MapSize);
    for (const auto row : iterator)
    {
        for (const auto col : iterator)
        {
            if (At(row, col).isValid)
            {
                result.push_back(At(row, col));
            }
        }
    }
    result.shrink_to_fit();
    return result;
}

bool MapTiles::ValidIndex(const int row, const int col)
{
    return (row >= -MaxTilesFromCenter && row <= MaxTilesFromCenter && col >= -MaxTilesFromCenter && col <= MaxTilesFromCenter);
}
