// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef ICONBUFFER_H_INCLUDED
#define ICONBUFFER_H_INCLUDED

#include <vector>
#include <memory>
#include <zen/zstring.h>
#include <zen/optional.h>
#include <wx/bitmap.h>

namespace zen
{
class IconBuffer
{
public:
    enum IconSize
    {
        SIZE_SMALL,
        SIZE_MEDIUM,
        SIZE_LARGE
    };

    IconBuffer(IconSize sz);
    ~IconBuffer();

    static int getSize(IconSize icoSizeType); //expected and *maximum* icon size in pixel
    int getSize() const { return getSize(iconSizeType); } //

    const wxBitmap& genericFileIcon() { return genFileIcon; }
    const wxBitmap& genericDirIcon () { return genDirIcon;  }

    bool readyForRetrieval(const Zstring& filename);
    Opt<wxBitmap> retrieveFileIcon(const Zstring& filename); //... and mark as hot

    void setWorkload(const std::vector<Zstring>& load); //(re-)set new workload of icons to be retrieved;

private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;

    const IconSize iconSizeType;
    const wxBitmap genDirIcon;
    const wxBitmap genFileIcon;
};
}

#endif // ICONBUFFER_H_INCLUDED
