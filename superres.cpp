#include "superres.h"

// #include "snes9x.h"
// #include "ppu.h"
// #include "tile.h"

#include <unordered_map>
#include <string>
#include <iostream>

// #include "ppu.h"

std::unordered_map<std::string, std::unordered_map<std::string, int>*> palettesPerTile;
std::unordered_map<std::string, int> uniqueTilePalettePairs;
std::unordered_map<std::string, int> uniquePalettes;
// std::unordered_map<std::string, int> uniqueTiles;


void DumpTileWithPalette(uint8 * pCache)
{
    for (uint32 i=0;i!=64;++i)
        XGFX.DumpedTileWithPalette[i] = pCache[i];
    
    uint16 * const palette = (uint16*) &XGFX.DumpedTileWithPalette[64];
    
    for (uint32 i=0;i!=BG.PaletteSize;++i)
        palette[i] = GFX.ScreenColors[BG.StartPalette+i];
    
    XGFX.DumpedTileWithPaletteSize = 64u + BG.PaletteSize * 2u;
}

void CountReferences()
{
    std::string tileStr((const char*)XGFX.DumpedTileWithPalette, 64);
    std::string paletteStr((const char*)(XGFX.DumpedTileWithPalette+64), XGFX.DumpedTileWithPaletteSize-64);
    std::string tilePaletteStr((const char*)XGFX.DumpedTileWithPalette, XGFX.DumpedTileWithPaletteSize);

    // auto it1 = uniqueTiles.find(tileStr);
    // if (it1==uniqueTiles.end()) uniqueTiles[tileStr] = 1;
    // else it1->second += 1;
    
    auto it2 = uniquePalettes.find(paletteStr);
    if (it2==uniquePalettes.end()) uniquePalettes[paletteStr] = 1;
    else it2->second += 1;
    
    auto it3 = uniqueTilePalettePairs.find(tilePaletteStr);
    if (it3==uniqueTilePalettePairs.end()) uniqueTilePalettePairs[tilePaletteStr] = 1;
    else it3->second += 1;
    
    auto it4 = palettesPerTile.find(tileStr);
    if (it4==palettesPerTile.end()) 
    {
        auto tmp = new std::unordered_map<std::string, int>();
        (*tmp)[paletteStr] = 1;
        palettesPerTile[tileStr] = tmp;
    }
    else 
    {
        auto it5 = it4->second->find(paletteStr);
        if (it5==it4->second->end()) (*it4->second)[paletteStr] = 1;
        else it5->second += 1;
    }
}

void ClearReferenceCounters()
{
    palettesPerTile.clear();
    uniquePalettes.clear();
    uniqueTilePalettePairs.clear();
}

/*
    Super Metroid, quick counter (all demos in the beginning plus a small walk)
    Unique tiles: 6107
    Unique palettes: 4205
    Unique pairs: 80988
*/

void ShowReferenceCounters()
{
    std::cout << "Unique tiles: " << palettesPerTile.size() << std::endl;
    std::cout << "Unique palettes: " << uniquePalettes.size() << std::endl;
    std::cout << "Unique pairs: " << uniqueTilePalettePairs.size() << std::endl;
}
