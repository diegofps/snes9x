#include "xgfx.h"

#include <unordered_map>
#include <string>
#include <iostream>
#include <fstream>

#include "snes9x.h"
#include "ppu.h"


void xgfxInit()
{
    std::cout << "Initializing xgfx" << std::endl;
    
    XGFX.TakeReferenceScreenshot = FALSE;
    XGFX.DrawingOBJS = FALSE;
    XGFX.ScreenshotID = 1;
    XGFX.ReferenceID = 1;
    XGFX.PaletteID = 1;
    XGFX.TileID = 1;

    for (auto pair : XGFX.DumpedPalettes)
        delete pair.second;

    for (auto pair : XGFX.DumpedTiles)
        delete pair.second;

    for (auto ref : XGFX.DumpedReferences)
        delete ref;

    XGFX.DumpedReferences.clear();
    XGFX.DumpedPalettes.clear();
    XGFX.DumpedTiles.clear();

    // TODO: Clear the dump folder
}

void xgfxCaptureTileAndPalette(
    uint32 Tile,
    uint8 * pCache, 
    uint32 StartLine, 
    uint32 LineCount, 
    uint32 Offset,
    const char * MATH,
	const char * PIXEL,
	const char * OP,
	const char * BPSTART,
	const char * TILE)
{
    PaletteDump * palette = nullptr;
    TileDump * tile = nullptr;

    // Create the tile and colorPalette dictionary keys

    XGFX.DumpedTileKey.resize(64);
    memcpy(&XGFX.DumpedTileKey[0], pCache, 64);

    XGFX.DumpedPaletteKey.resize(BG.PaletteSize*2);
    memcpy(&XGFX.DumpedPaletteKey[0], GFX.ScreenColors, XGFX.DumpedPaletteKey.size());

    // This will initialize a reference screenshot

    auto createReference = [&](uint32 const tileID, uint32 const colorPaletteID)
    {
        auto ref = new ReferenceDump();
        ref->ID = XGFX.ReferenceID++;
        ref->TileID = tileID;
        ref->ScreenshotID = XGFX.ScreenshotID;
        ref->Frame = IPPU.TotalEmulatedFrames;
        ref->X = Offset%GFX.RealPPL;
        ref->Y = Offset/GFX.RealPPL-StartLine;
        ref->StartLine = StartLine;
        ref->LineCount = LineCount;
        ref->ColorPaletteID = colorPaletteID;
        ref->MATH = MATH;
        ref->PIXEL = PIXEL;
        ref->OP = OP;
        ref->BPSTART = BPSTART;
        ref->TILE = TILE;
        XGFX.DumpedReferences.push_back(ref);
        XGFX.TakeReferenceScreenshot = TRUE;
    };

    // Find or create the color palette

    auto itPalette = XGFX.DumpedPalettes.find(XGFX.DumpedPaletteKey);

    if (itPalette == XGFX.DumpedPalettes.end())
    {
        palette = new PaletteDump(XGFX.PaletteID++, XGFX.DumpedPaletteKey);
        XGFX.DumpedPalettes[XGFX.DumpedPaletteKey] = palette;
    }
    else
    {
        palette = itPalette->second;
        ++palette->Frequency;
    }

    // Find of create the tile

    auto it = XGFX.DumpedTiles.find(XGFX.DumpedTileKey);

    if (it == XGFX.DumpedTiles.end())
    {
        tile = new TileDump();
        tile->Tile = Tile;
        tile->ID = XGFX.TileID++;
        memcpy(tile->Pixels, &XGFX.DumpedTileKey[0], XGFX.DumpedTileKey.size());
        tile->PaletteSize = BG.PaletteSize;
        tile->LastSeenOnFrame = IPPU.TotalEmulatedFrames;
        tile->SeenOnFrames = 1;
        tile->UsedInBackground = FALSE;
        tile->UsedInSprite = FALSE;
        tile->UsedWithHFlip = FALSE;
        tile->UsedWithVFlip = FALSE;

        createReference(tile->ID, palette->ID);
        XGFX.DumpedTiles[XGFX.DumpedTileKey] = tile;
    }
    else
    {
        tile = it->second;
    }

    // Update the flags that indicate if this tile was drawed for a sprite or a background, HFlip, or VFlip.

    if (XGFX.DrawingOBJS==TRUE)
        tile->UsedInSprite = TRUE;
    else
        tile->UsedInBackground = TRUE;

    if (Tile & H_FLIP)
        tile->UsedWithHFlip = TRUE;

    if (Tile & V_FLIP)
        tile->UsedWithVFlip = TRUE;

    // Check if it is time to capture more reference screenshots

    if (tile->LastSeenOnFrame != IPPU.TotalEmulatedFrames)
    {
        tile->LastSeenOnFrame = IPPU.TotalEmulatedFrames;
        tile->SeenOnFrames++;

        if (tile->SeenOnFrames == 10)
            createReference(tile->ID, palette->ID);

        else if (tile->SeenOnFrames == 100)
            createReference(tile->ID, palette->ID);
    }

    // Update the palettes used by this tile

    ++tile->PalettesUsed[palette->ID];

    // This should never happen, but let's check if it ever happens
    // This is common and happens a lot
    // if (BG.PaletteSize != tile->PaletteSize)
    //     std::cout << "INFO: Found a tile that uses color palettes with different sizes" << std::endl;
}

void xgfxDumpPaletteAsJson(PaletteDump & palette, std::ostream & o, std::string indent)
{
    int r,g,b;

    o << indent << "{";

    o << "\"ID\": " << palette.ID;
    o << ", \"Size\": " << palette.Size;
    o << ", \"Frequency\": " << palette.Frequency;

    o << ", \"Colors\": [";

    DECOMPOSE_PIXEL_8_BITS(palette.Colors[0],r,g,b);
    o << "[" << r << "," << g << "," << b << "]";

    for (uint32 i=1;i!=palette.Size;++i)
    {
        DECOMPOSE_PIXEL_8_BITS(palette.Colors[i],b,g,r);
        o << ",[" << r << "," << g << "," << b << "]";
    }
    o << "]";

    o << "}";
}

void xgfxDumpReferenceAsJson(ReferenceDump & ref, std::ostream & o, std::string indent)
{
    o << indent << "{";
    o << "\"ID\": " << ref.ID;
    o << "," << " \"TileID\": " << ref.TileID;
    o << "," << " \"ScreenshotID\": " << ref.ScreenshotID;
    o << "," << " \"Frame\": " << ref.Frame;
    o << "," << " \"X\": " << ref.X;
    o << "," << " \"Y\": " << ref.Y;
    o << "," << " \"StartLine\": " << ref.StartLine;
    o << "," << " \"LineCount\": " << ref.LineCount;
    o << "," << " \"MATH\": \"" << ref.MATH << "\"";
    o << "," << " \"PIXEL\": \"" << ref.PIXEL << "\"";
    o << "," << " \"OP\": \"" << ref.OP << "\"";
    o << "," << " \"BPSTART\": \"" << ref.BPSTART << "\"";
    o << "," << " \"TILE\": \"" << ref.TILE << "\"";
    o << "," << " \"ColorPaletteID\": " << ref.ColorPaletteID;
    o << "}";
}

void xgfxDumpTileAsJson(TileDump & tile, std::ostream & o, std::string indent)
{
    o << indent << "{";

    o << std::endl << indent << "  \"ID\": " << tile.ID;
    o << "," << std::endl << indent << "  \"Tile\": " << tile.Tile;

    o << "," << std::endl << indent << "  \"Pixels\": [" << int(tile.Pixels[0]);
    for (uint32 i=1;i!=64;++i)
        o << "," << int(tile.Pixels[i]);
    o << "]";

    o << "," << std::endl << indent << "  \"Palettes\": {";
    bool8 first = TRUE;
    for (auto pair : tile.PalettesUsed)
    {
        if (first) first = FALSE;
        else o << ", ";
        o << "\"" << pair.first << "\":" << pair.second;
    }
    o << "}";

    o << "," << std::endl << indent << "  \"PaletteSize\": " << tile.PaletteSize;
    o << "," << std::endl << indent << "  \"LastSeenOnFrame\": " << tile.LastSeenOnFrame;
    o << "," << std::endl << indent << "  \"SeenOnFrames\": " << tile.SeenOnFrames;
    o << "," << std::endl << indent << "  \"PalettesSeen\": " << tile.PalettesUsed.size();
    o << "," << std::endl << indent << "  \"UsedInBackground\": " << (tile.UsedInBackground==TRUE?"true":"false");
    o << "," << std::endl << indent << "  \"UsedInSprite\": " << (tile.UsedInSprite==TRUE?"true":"false");
    o << "," << std::endl << indent << "  \"UsedWithHFlip\": " << (tile.UsedWithHFlip==TRUE?"true":"false");
    o << "," << std::endl << indent << "  \"UsedWithVFlip\": " << (tile.UsedWithVFlip==TRUE?"true":"false");

    o << std::endl << indent << "}";
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
        xgfxDumpTileAsJson(*pair.second, o, "  ");
    }

    o << std::endl << "]";
}

void xgfxDumpPalettesAsJson(std::ostream & o)
{
    o << "[";
    bool8 first = TRUE;

    for (auto pair : XGFX.DumpedPalettes)
    {
        if (first)
            first = FALSE;
        else
            o << ",";
        
        o << std::endl;
        xgfxDumpPaletteAsJson(*pair.second, o, "  ");
    }

    o << std::endl << "]";
}

void xgfxDumpReferencesAsJson(std::ostream & o)
{
    o << "[";
    bool8 first = TRUE;

    for (auto ref : XGFX.DumpedReferences)
    {
        if (first)
            first = FALSE;
        else
            o << ",";
        
        o << std::endl;
        xgfxDumpReferenceAsJson(*ref, o, "  ");
    }

    o << std::endl << "]";
}

/*
    Super Metroid, quick counter (all demos in the beginning plus a small walk)
    Unique tiles: 6107
*/

void xgfxShowReferenceCounters()
{
    std::cout << "Unique tiles: " << XGFX.DumpedTiles.size() << std::endl;
    std::cout << "Unique palettes: " << XGFX.DumpedPalettes.size() << std::endl;
    std::cout << "References: " << XGFX.DumpedReferences.size() << std::endl;

    // std::cout << std::endl;
    // std::cout << "Unique tiles: " << palettesPerTile.size() << std::endl;
    // std::cout << "Unique palettes: " << uniquePalettes.size() << std::endl;
    // std::cout << "Unique pairs: " << uniqueTilePalettePairs.size() << std::endl;
}

