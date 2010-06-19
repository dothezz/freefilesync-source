// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef ICONBUFFER_H_INCLUDED
#define ICONBUFFER_H_INCLUDED

#include <vector>
#include "../shared/zstring.h"
#include <memory>

class wxCriticalSection;
class wxIcon;


namespace FreeFileSync
{

class IconBuffer
{
public:
    static const wxIcon& getDirectoryIcon(); //one folder icon should be sufficient...

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

    static const size_t BUFFER_SIZE = 800; //maximum number of icons to buffer

    class IconDB;
    class IconHolder;
    class IconDbSequence;

    //methods used by worker thread
    void insertIntoBuffer(const DefaultChar* entryName, const IconHolder& icon);

    static IconHolder getAssociatedIcon(const Zstring& filename);
    static IconHolder getAssociatedIconByExt(const Zstring& extension);

//---------------------- Shared Data -------------------------
    std::auto_ptr<wxCriticalSection> lockIconDB;
    std::auto_ptr<IconDB> buffer;  //use synchronisation when accessing this!
    std::auto_ptr<IconDbSequence> bufSequence; //save sequence of buffer entry to delete oldest elements (implicitly shared by sharing Zstring with IconDB!!!)
//------------------------------------------------------------

    class WorkerThread;
    WorkerThread* worker; //detached thread => destroys itself
};
}

#endif // ICONBUFFER_H_INCLUDED
