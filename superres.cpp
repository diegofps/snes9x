#include "superres.h"

#include <unordered_map>
#include <string>
#include <iostream>
#include <fstream>

#include "snes9x.h"
#include "ppu.h"


void xgfxInit()
{
    std::cout << "Initializing xgfx" << std::endl;
}

void xgfxCaptureTileAndPalette(uint8 * pCache, uint32 LineCount, uint32 Offset)
{
    uint32 k=LineCount*8;
    XGFX.DumpedTileKey.resize(k);
    memcpy(&XGFX.DumpedTileKey[0], pCache, k);

    XGFX.DumpedPaletteKey.resize(BG.PaletteSize*2);
    memcpy(&XGFX.DumpedPaletteKey[0], GFX.ScreenColors, XGFX.DumpedPaletteKey.size());

    auto it = XGFX.DumpedTiles.find(XGFX.DumpedTileKey);
    TileDump * tile = nullptr;

    if (it == XGFX.DumpedTiles.end())
    {
        tile = new TileDump();
        memcpy(tile->Pixels, &XGFX.DumpedTileKey[0], XGFX.DumpedTileKey.size());
        tile->Lines = XGFX.DumpedTileKey.size() / 8;
        tile->PaletteSize = BG.PaletteSize;

        tile->RefScreenshot1 = XGFX.ReferenceScreenshotId;
        tile->RefFrame1 = IPPU.TotalEmulatedFrames;
        tile->RefX1 = Offset%GFX.RealPPL;
        tile->RefY1 = Offset/GFX.RealPPL;

        tile->RefScreenshot2 = -1;
        tile->RefFrame2 = -1;
        tile->RefX2 = 0;
        tile->RefY2 = 0;

        tile->RefScreenshot3 = -1;
        tile->RefFrame3 = -1;
        tile->RefX3 = 0;
        tile->RefY3 = 0;

        tile->LastSeenOnFrame = IPPU.TotalEmulatedFrames;
        tile->SeenOnFrames = 1;
        XGFX.TakeReferenceScreenshot = TRUE;

        XGFX.DumpedTiles[XGFX.DumpedTileKey] = tile;
    }
    else
    {
        tile = it->second;
    }

    if (tile->LastSeenOnFrame != IPPU.TotalEmulatedFrames)
    {
        tile->LastSeenOnFrame = IPPU.TotalEmulatedFrames;
        tile->SeenOnFrames++;

        if (tile->SeenOnFrames == 10)
        {
            tile->RefFrame2 = IPPU.TotalEmulatedFrames;
            tile->RefScreenshot2 = XGFX.ReferenceScreenshotId;
            tile->RefX2 = Offset%GFX.RealPPL;
            tile->RefY2 = Offset/GFX.RealPPL;
            XGFX.TakeReferenceScreenshot = TRUE;
        }

        else if (tile->SeenOnFrames == 100)
        {
            tile->RefFrame3 = IPPU.TotalEmulatedFrames;
            tile->RefScreenshot3 = XGFX.ReferenceScreenshotId;
            tile->RefX3 = Offset%GFX.RealPPL;
            tile->RefY3 = Offset/GFX.RealPPL;
            XGFX.TakeReferenceScreenshot = TRUE;
        }
    }

    if (BG.PaletteSize != tile->PaletteSize)
        std::cout << "INFO: Found a tile that uses color palettes with different sizes" << std::endl;

    auto it2 = tile->PalettesUsed.find(XGFX.DumpedPaletteKey);
    if (it2 == tile->PalettesUsed.end())
		tile->PalettesUsed[XGFX.DumpedPaletteKey] = new PaletteDump(XGFX.DumpedPaletteKey);
    else
        ++it2->second->Frequency;
}

void xgfxDumpPaletteAsJson(PaletteDump & palette, std::ostream & o)
{
    int r,g,b;

    o << "{";

    o << std::endl << "  \"Colors\": [";

    DECOMPOSE_PIXEL_8_BITS(palette.Colors[0],r,g,b);
    o << "[" << r << "," << g << "," << b << "]";

    for (uint32 i=1;i!=palette.Size;++i)
    {
        DECOMPOSE_PIXEL_8_BITS(palette.Colors[i],r,g,b);
        o << ", [" << r << "," << g << "," << b << "]";
    }
    o << "]";

    o << "," << std::endl << "  \"Size\": " << palette.Size;
    o << "," << std::endl << "  \"Frequency\": " << palette.Frequency;

    o << "}";
}

void xgfxDumpTileAsJson(TileDump & tile, std::ostream & o)
{
    o << "{";
    
    o << std::endl << "  \"Pixels\": [" << int(tile.Pixels[0]);
    for (uint32 i=1;i!=tile.Lines*8;++i)
        o << "," << int(tile.Pixels[i]);
    o << "]";

    o << "," << std::endl << "  \"Lines\": " << tile.Lines;
    o << "," << std::endl << "  \"PaletteSize\": " << tile.PaletteSize;
    o << "," << std::endl << "  \"RefScreenshot1\": { \"id\":" << tile.RefScreenshot1 << ", \"frame\":" << tile.RefFrame1 << ", \"x\":" << tile.RefX1 << ", \"y\":" << tile.RefY1 << "}";
    o << "," << std::endl << "  \"RefScreenshot2\": { \"id\":" << tile.RefScreenshot2 << ", \"frame\":" << tile.RefFrame2 << ", \"x\":" << tile.RefX2 << ", \"y\":" << tile.RefY2 << "}";
    o << "," << std::endl << "  \"RefScreenshot3\": { \"id\":" << tile.RefScreenshot3 << ", \"frame\":" << tile.RefFrame3 << ", \"x\":" << tile.RefX3 << ", \"y\":" << tile.RefY3 << "}";
    o << "," << std::endl << "  \"LastSeenOnFrame\": " << tile.LastSeenOnFrame;
    o << "," << std::endl << "  \"SeenOnFrames\": " << tile.SeenOnFrames;
    o << "," << std::endl << "  \"PalettesSeen\": " << tile.PalettesUsed.size();

    o << "," << std::endl << "  \"Palettes\": [";
    bool8 first = TRUE;
    for (auto pair : tile.PalettesUsed)
    {
        if (first)
            first = FALSE;
        else 
            o << ",";
        
        o << std::endl;
        xgfxDumpPaletteAsJson(*pair.second, o);
    }

    o << std::endl << "]";

    o << std::endl << "}";
}

void xgfxDumpTilesAsJson(std::ostream & o)
{
    o << "[";
    bool8 first = TRUE;

    for (auto pair : XGFX.DumpedTiles)
    {
        if (first)
            first = FALSE;
        else
            o << ",";
        
        o << std::endl;
        xgfxDumpTileAsJson(*pair.second, o);
    }

    o << std::endl << "]";
}

/*
    Super Metroid, quick counter (all demos in the beginning plus a small walk)
    Unique tiles: 6107
    Unique palettes: 4205
    Unique pairs: 80988
*/

void xgfxShowReferenceCounters()
{
    std::cout << "Unique tiles: " << XGFX.DumpedTiles.size() << std::endl;

    // std::cout << std::endl;
    // std::cout << "Unique tiles: " << palettesPerTile.size() << std::endl;
    // std::cout << "Unique palettes: " << uniquePalettes.size() << std::endl;
    // std::cout << "Unique pairs: " << uniqueTilePalettePairs.size() << std::endl;
}

