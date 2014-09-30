// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef OSX_FILE_ICON_8427508422345342
#define OSX_FILE_ICON_8427508422345342

#include <vector>
#include <zen/sys_error.h>

namespace osx
{
struct ImageData
{
    ImageData(int w, int h) : width(w), height(h), rgb(w* h * 3), alpha(w* h) {}
    ImageData(ImageData&& tmp) : width(tmp.width), height(tmp.height) { rgb.swap(tmp.rgb); alpha.swap(tmp.alpha); }

    const int width;
    const int height;
    std::vector<unsigned char> rgb; //rgb-byte order for use with wxImage
    std::vector<unsigned char> alpha;

private:
    ImageData(const ImageData&);
    ImageData& operator=(const ImageData&);
};

ImageData getThumbnail(const char* filename, int requestedSize); //throw SysError
ImageData getFileIcon (const char* filename, int requestedSize); //throw SysError
ImageData getDefaultFileIcon  (int requestedSize); //throw SysError
ImageData getDefaultFolderIcon(int requestedSize); //throw SysError
}

#endif //OSX_FILE_ICON_8427508422345342
