#include "pch.h"

#include "tilemap.h"

auto MapTiles::Init() -> void
{
    for (const auto row : iterator)
    {
        for (const auto col : iterator)
        {
            auto& currenTile {m_map.at(row + MaxTilesFromCenter + (col + MaxTilesFromCenter) * Width)};
            currenTile = {.row = static_cast<int8_t>(row), .col = static_cast<int8_t>(col)};
                
            if (row == 0 && col == 0)
            {
                currenTile.isValid = true;
                continue;
            }
            currenTile.isValid = false;
        }
    }
}

auto MapTiles::At(const int row, const int col) -> MapTile&
{
    assert(ValidIndex(row, col));
    return m_map.at(row + MaxTilesFromCenter + (col + MaxTilesFromCenter) * Width);
}

auto MapTiles::SetValid(const int row, const int col) -> void
{
    if (!ValidIndex(row, col))
    {
        return;
    }

    auto& currenTile {At(row, col)};
    currenTile.isValid = !currenTile.isValid;
}

auto MapTiles::SetEntity(const Entity entity, const int row, const int col) -> void
{
    if (!ValidIndex(row, col))
    {
        return;
    }

    auto& currenTile {At(row, col)};
    currenTile.entity = entity;
}

auto MapTiles::IsEmpty() -> bool
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

auto MapTiles::ValidTiles() -> std::vector<MapTile>
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

auto MapTiles::ValidIndex(const int row, const int col) -> bool
{
    return (row >= -MaxTilesFromCenter && row <= MaxTilesFromCenter && col >= -MaxTilesFromCenter && col <= MaxTilesFromCenter);
}
