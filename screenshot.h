/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#ifndef _SCREENSHOT_H_
#define _SCREENSHOT_H_

std::string S9xContextualizeFilename(std::string contextName, std::string fileName);
bool8 S9xDoScreenshot (int width, int height);
bool8 S9xDoReferenceScreenshot (int width, int height);
bool8 S9xDoMovieScreenshot (int width, int height);

#endif
