#ifndef _SUPERRES_H_
#define _SUPERRES_H_

// #include "snes9x.h"
// #include "ppu.h"
#include "port.h"

void xgfxInit();
void xgfxCaptureTileAndPalette(uint8 * pCache, uint32 LineCount, uint32 Offset);
void xgfxDumpTilesAsJson(std::ostream & o);
void xgfxShowReferenceCounters();

#endif /* _SUPERRES_H_ */