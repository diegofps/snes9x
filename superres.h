#ifndef _SUPERRES_H_
#define _SUPERRES_H_

#include "snes9x.h"
#include "ppu.h"

void DumpTileWithPalette(uint8 * pCache);
void ClearReferenceCounters();
void CountReferences();
void ShowReferenceCounters();

#endif /* _SUPERRES_H_ */