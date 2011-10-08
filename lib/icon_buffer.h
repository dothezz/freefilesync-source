// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
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

    int getSize() const { return cvrtSize(icoSize); } //*maximum* icon size in pixel

    const wxIcon& genericDirIcon () { return genDirIcon;  }
    const wxIcon& genericFileIcon() { return genFileIcon; }

    bool requestFileIcon(const Zstring& filename, wxIcon* icon = NULL); //returns false if icon is not in buffer
    void setWorkload(const std::vector<Zstring>& load); //(re-)set new workload of icons to be retrieved;

    static int cvrtSize(IconSize sz);

private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;

    const IconSize icoSize;
    const wxIcon genDirIcon;
    const wxIcon genFileIcon;
};
}

#endif // ICONBUFFER_H_INCLUDED
