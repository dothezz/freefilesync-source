// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef ICONBUFFER_H_INCLUDED
#define ICONBUFFER_H_INCLUDED

#include <memory>
#include <wx/icon.h>
#include <zen/zstring.h>


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

    const wxIcon& genericFileIcon() { return genFileIcon; }
    const wxIcon& genericDirIcon () { return genDirIcon;  }

    int getSize() const; //*maximum* icon size in pixel

    bool requestFileIcon(const Zstring& filename, wxIcon* icon = nullptr); //returns false if icon is not in buffer
    void setWorkload(const std::vector<Zstring>& load); //(re-)set new workload of icons to be retrieved;

private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;

    const IconSize icoSize;
    const wxIcon genDirIcon;
    const wxIcon genFileIcon;
};
}

#endif // ICONBUFFER_H_INCLUDED
