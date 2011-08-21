// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef ICONBUFFER_H_INCLUDED
#define ICONBUFFER_H_INCLUDED

#include "../shared/zstring.h"
#include <memory>
#include <wx/icon.h>


namespace zen
{
class IconBuffer
{
public:
    static const wxIcon& getDirectoryIcon(); //generic icons
    static const wxIcon& getFileIcon();      //

    static IconBuffer& getInstance();
    bool requestFileIcon(const Zstring& fileName, wxIcon* icon = NULL); //returns false if icon is not in buffer
    void setWorkload(const std::vector<Zstring>& load); //(re-)set new workload of icons to be retrieved;

#ifdef FFS_WIN
    static const int ICON_SIZE = 16;  //size in pixel
#elif defined FFS_LINUX
    static const int ICON_SIZE = 24;  //size in pixel
#endif

private:
    IconBuffer();
    ~IconBuffer();

    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};
}

#endif // ICONBUFFER_H_INCLUDED
