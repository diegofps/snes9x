/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#ifndef _GFX_H_
#define _GFX_H_

#include "port.h"
#include <vector>
#include <unordered_map>

struct SGFX // The Virtual Screen, ZBuffer, screen color palettes, etc
{
	const uint32 Pitch = sizeof(uint16) * MAX_SNES_WIDTH;
	const uint32 RealPPL = MAX_SNES_WIDTH; // true PPL of Screen buffer, Pixels Per Line?
	const uint32 ScreenSize =  MAX_SNES_WIDTH * MAX_SNES_HEIGHT;

	std::vector<uint16> ScreenBuffer; // The screen buffer + 64 lines, .resize(MAX_SNES_WIDTH * (MAX_SNES_HEIGHT + 64))
	uint16	*Screen;     // The main screen buffer. Points to the screen buffer + 32 lines, = &GFX.ScreenBuffer[GFX.RealPPL * 32]
	uint16	*SubScreen;  // The sub screen buffer. Size: ScreenSize * sizeof(uint16)
	uint8	*ZBuffer;    // The depth buffer for the main screen. Size: ScreenSize
	uint8	*SubZBuffer; // The depth buffer for the sub screen. Size: ScreenSize
	uint16	*S;          // The screen or subscreen buffer, = Screen, = SubScreen, += RealPPL, Screen + StartY * PPL
	uint8	*DB;		 // The ZBuffer or SubZBuffer, i.e., the pixel depths for the screen being drawn on. "DepthBuffer", = ZBuffer, = SubZBuffer (This will receive the Z2 value when the pixel is drawn)
	uint16	*ZERO;		 // Lookup table for 1/2 color subtraction. Size: sizeof(uint16) * 0x10000
	uint32	PPL;				// Number of pixels in a line of the Screen buffer considering interlace. When interlace is on, this will be twice the RealPPL.
	uint32	LinesPerTile;		// Number of lines in a tile (4 or 8 due to interlace)
	uint16	*ScreenColors;		// Screen colors for rendering main / This is a color palette
	uint16	*RealScreenColors;	// Screen colors, ignoring color window clipping
	uint8	Z1;					// Depth of tile for comparison with DB - draw if Z1 > DB
	uint8	Z2;					// depth of tile to save in DB after drawing
	uint32	FixedColour;   // Used during blend (BLEND_ADD, BLEND_SUB, and BLEND_SATURATION) as the second color parameter. It comes from PPU.FixedColour(Red|Green|Blue).
	uint8	DoInterlace;
	uint32	StartY;
	uint32	EndY;
	bool8	ClipColors;            // This is true when the first bit of GFX.Clip[bg].DrawMode[clip - 1] is zero
	uint8	OBJWidths[128];
	uint8	OBJVisibleTiles[128];

	struct ClipData	*Clip;

	struct
	{
		uint8	RTOFlags;
		int16	Tiles;

		struct
		{
			int8	Sprite;
			uint8	Line;
		}	OBJ[128];
	}	OBJLines[SNES_HEIGHT_EXTENDED];

	void	(*DrawBackdropMath) (uint32, uint32, uint32);
	void	(*DrawBackdropNomath) (uint32, uint32, uint32);
	void	(*DrawTileMath) (uint32, uint32, uint32, uint32);
	void	(*DrawTileNomath) (uint32, uint32, uint32, uint32);
	void	(*DrawClippedTileMath) (uint32, uint32, uint32, uint32, uint32, uint32);
	void	(*DrawClippedTileNomath) (uint32, uint32, uint32, uint32, uint32, uint32);
	void	(*DrawMosaicPixelMath) (uint32, uint32, uint32, uint32, uint32, uint32);
	void	(*DrawMosaicPixelNomath) (uint32, uint32, uint32, uint32, uint32, uint32);
	void	(*DrawMode7BG1Math) (uint32, uint32, int);
	void	(*DrawMode7BG1Nomath) (uint32, uint32, int);
	void	(*DrawMode7BG2Math) (uint32, uint32, int);
	void	(*DrawMode7BG2Nomath) (uint32, uint32, int);

	std::string InfoString;
	uint32	InfoStringTimeout;
	char	FrameDisplayString[256];
};

struct PaletteDump;
struct TileDump;
struct SXGFX;

struct SXGFX // The High Definition Virtual Screen
{
	std::unordered_map<std::string, TileDump*> DumpedTiles;
	std::string DumpedPaletteKey;
	std::string DumpedTileKey;

	// uint8   *DumpedTileWithPalette;	    // This will hold the pair (tile, colorPalette)
	// uint32  DumpedTileWithPaletteSize;  // This is the size of of the dumped pair, tile and palette
	bool8   TakeReferenceScreenshot;    // Used to capture a screenshot when a new pair of tile and palette was found. Used as a reference to know where it came from.
	uint32  ReferenceScreenshotId;      // Number of reference screenshot. This is saved in the screenshot name and the captured tile sample.
};

struct PaletteDump
{
	uint16  Colors[256];  // A copy of the first colorPalette used
	uint32  Size;         // The number of colors in this color palette (up to 256)
	uint32  Frequency;    // The number of times this color palette was used (may be used multiple times per frame)

	PaletteDump(std::string & colorPalette)
	{
		memcpy(Colors, &colorPalette[0], colorPalette.size());
		Size = colorPalette.size() / 2;
		Frequency = 1;
	}

};

struct TileDump // A struct to capture and dump tiles during the game execution
{
	std::unordered_map<std::string, PaletteDump*> PalettesUsed; // Memorizes the palette colors and frequency they were used

	uint8   Pixels[64];            // The pixel colors before conversion to RGB, represented as color palette indexes.
	uint32  Lines;                 // The number of lines it was called to draw. Probably 8, 4, or 2.
	uint32  PaletteSize;           // The number of colors it uses in the palette. May be 1<<2, 1<<4, or 1<<8.
	int32   RefScreenshot1;  // Number of first frame it appeared, as XGFX.ReferenceScreenshotId
	int32   RefScreenshot2;  // Number of 10th frame it appeared, as XGFX.ReferenceScreenshotId (may be -1 if not seen for 10 frames)
	int32   RefScreenshot3;  // Number of 100th frame it appeared, as XGFX.ReferenceScreenshotId (may be -1 if not seen for 100 frames)
	int32   RefFrame1;     // Number of first frame it appeared, as IPPU.TotalEmulatedFrames
	int32   RefFrame2;     // Number of 10th frame it appeared , as IPPU.TotalEmulatedFrames. May be -1 if not seen for 10 frames.
	int32   RefFrame3;     // Number of 100th frame it appeared , as IPPU.TotalEmulatedFrames. May be -1 if not seen for 100 frames.
	int32   RefX1, RefY1;    // Coordinate in RefScreenshot1
	int32   RefX2, RefY2;    // Coordinate in RefScreenshot2
	int32   RefX3, RefY3;    // Coordinate in RefScreenshot3
	uint32  LastSeenOnFrame;       // Number from IPPU.TotalEmulatedFrames
	uint32  SeenOnFrames;          // Number of different frames it was seen, used to capture reference screenshots;

};

struct SBG
{
	uint8	(*ConvertTile) (uint8 *, uint32, uint32);
	uint8	(*ConvertTileFlip) (uint8 *, uint32, uint32);

	uint32	TileSizeH;
	uint32	TileSizeV;
	uint32	OffsetSizeH;
	uint32	OffsetSizeV;
	uint32	TileShift;
	uint32	TileAddress;
	uint32	NameSelect;
	uint32	SCBase;

	uint32	StartPalette;
	uint32	PaletteSize;
	uint32	PaletteShift;
	uint32	PaletteMask;
	uint8	EnableMath;
	uint8	InterlaceLine;

	uint8	*Buffer;
	uint8	*BufferFlip;
	uint8	*Buffered;
	uint8	*BufferedFlip;
	bool8	DirectColourMode;
};

struct SLineData
{
	struct
	{
		uint16	VOffset;
		uint16	HOffset;
	}	BG[4];
};

struct SLineMatrixData
{
	short	MatrixA;
	short	MatrixB;
	short	MatrixC;
	short	MatrixD;
	short	CentreX;
	short	CentreY;
	short	M7HOFS;
	short	M7VOFS;
};

extern uint16		BlackColourMap[256];
extern uint16		DirectColourMaps[8][256];
extern uint8		mul_brightness[16][32];
extern uint8		brightness_cap[64];
extern struct SBG	BG;
extern struct SGFX	GFX;
extern struct SXGFX	XGFX;

#define H_FLIP		0x4000
#define V_FLIP		0x8000
#define BLANK_TILE	2

/*
	These are used when the emulator is about to draw the pixels in the screen buffer. COLOR_ADD, 
	COLOR_SUB, and COLOR_ADD_BRIGHTNESS are the main blending techniques used to create transparency 
	and special effects. 

	* These structs will only calculate the pixel values. The pixels are drawn in tileimpl.h, using many other abstractions.
*/
struct COLOR_ADD
{
	// Used when GFX.ClipColors is true
	static alwaysinline uint16 fn(uint16 C1, uint16 C2)
	{
		const int RED_MASK = 0x1F << RED_SHIFT_BITS;
		const int GREEN_MASK = 0x1F << GREEN_SHIFT_BITS;
		const int BLUE_MASK = 0x1F;

		int rb = C1 & (RED_MASK | BLUE_MASK);
		rb += C2 & (RED_MASK | BLUE_MASK);
		int rbcarry = rb & ((0x20 << RED_SHIFT_BITS) | (0x20 << 0));
		int g = (C1 & (GREEN_MASK)) + (C2 & (GREEN_MASK));
		int rgbsaturate = (((g & (0x20 << GREEN_SHIFT_BITS)) | rbcarry) >> 5) * 0x1f;
		uint16 retval = (rb & (RED_MASK | BLUE_MASK)) | (g & GREEN_MASK) | rgbsaturate;
#if GREEN_SHIFT_BITS == 6
		retval |= (retval & 0x0400) >> 5;
#endif
		return retval;
	}

	// Used when GFX.ClipColors is false
	static alwaysinline uint16 fn1_2(uint16 C1, uint16 C2)
	{
		return ((((C1 & RGB_REMOVE_LOW_BITS_MASK) +
			(C2 & RGB_REMOVE_LOW_BITS_MASK)) >> 1) +
			(C1 & C2 & RGB_LOW_BITS_MASK)) | ALPHA_BITS_MASK;
	}
};

struct COLOR_ADD_BRIGHTNESS
{
	// Used when GFX.ClipColors is true
	static alwaysinline uint16 fn(uint16 C1, uint16 C2)
	{
		return ((brightness_cap[ (C1 >> RED_SHIFT_BITS)           +  (C2 >> RED_SHIFT_BITS)          ] << RED_SHIFT_BITS)   |
				(brightness_cap[((C1 >> GREEN_SHIFT_BITS) & 0x1f) + ((C2 >> GREEN_SHIFT_BITS) & 0x1f)] << GREEN_SHIFT_BITS) |
	// Proper 15->16bit color conversion moves the high bit of green into the low bit.
	#if GREEN_SHIFT_BITS == 6
			   ((brightness_cap[((C1 >> 6) & 0x1f) + ((C2 >> 6) & 0x1f)] & 0x10) << 1) |
	#endif
				(brightness_cap[ (C1                      & 0x1f) +  (C2                      & 0x1f)]      ));
	}

	// Used when GFX.ClipColors is false
	static alwaysinline uint16 fn1_2(uint16 C1, uint16 C2)
	{
		return COLOR_ADD::fn1_2(C1, C2);
	}
};


struct COLOR_SUB
{
	// Used when GFX.ClipColors is true
	static alwaysinline uint16 fn(uint16 C1, uint16 C2)
	{
		int rb1 = (C1 & (THIRD_COLOR_MASK | FIRST_COLOR_MASK)) | ((0x20 << 0) | (0x20 << RED_SHIFT_BITS));
		int rb2 = C2 & (THIRD_COLOR_MASK | FIRST_COLOR_MASK);
		int rb = rb1 - rb2;
		int rbcarry = rb & ((0x20 << RED_SHIFT_BITS) | (0x20 << 0));
		int g = ((C1 & (SECOND_COLOR_MASK)) | (0x20 << GREEN_SHIFT_BITS)) - (C2 & (SECOND_COLOR_MASK));
		int rgbsaturate = (((g & (0x20 << GREEN_SHIFT_BITS)) | rbcarry) >> 5) * 0x1f;
		uint16 retval = ((rb & (THIRD_COLOR_MASK | FIRST_COLOR_MASK)) | (g & SECOND_COLOR_MASK)) & rgbsaturate;
#if GREEN_SHIFT_BITS == 6
		retval |= (retval & 0x0400) >> 5;
#endif
		return retval;
	}

	// Used when GFX.ClipColors is false
	static alwaysinline uint16 fn1_2(uint16 C1, uint16 C2)
	{
		return GFX.ZERO[((C1 | RGB_HI_BITS_MASKx2) -
			(C2 & RGB_REMOVE_LOW_BITS_MASK)) >> 1];
	}
};

void S9xStartScreenRefresh (void);
void S9xEndScreenRefresh (void);
void S9xBuildDirectColourMaps (void);
void RenderLine (uint8);
void S9xComputeClipWindows (void);
void S9xDisplayChar (uint16 *, uint8);
void S9xGraphicsScreenResize (void);
// called automatically unless Settings.AutoDisplayMessages is false
void S9xDisplayMessages (uint16 *, int, int, int, int);

// external port interface which must be implemented or initialised for each port
bool8 S9xGraphicsInit (void);
void S9xGraphicsDeinit (void);
bool8 S9xInitUpdate (void);
bool8 S9xDeinitUpdate (int, int);
bool8 S9xContinueUpdate (int, int);
void S9xReRefresh (void);
void S9xSyncSpeed (void);

// called instead of S9xDisplayString if set to non-NULL
extern void (*S9xCustomDisplayString) (const char *, int, int, bool, int type);
void S9xVariableDisplayString(const char* string, int linesFromBottom, int pixelsFromLeft, bool allowWrap, int type);

#endif
