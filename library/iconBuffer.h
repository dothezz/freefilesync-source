// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef ICONBUFFER_H_INCLUDED
#define ICONBUFFER_H_INCLUDED

#ifndef FFS_WIN
header should be used in the windows build only!
#endif

#include <vector>
#include "../shared/zstring.h"
#include <memory>
#include <boost/shared_ptr.hpp>

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

    static const int    ICON_SIZE   = 16;  //size in pixel
    static const size_t BUFFER_SIZE = 800; //maximum number if icons to buffer

private:
    IconBuffer();
    ~IconBuffer();

    class WorkerThread;
    friend class WorkerThread;

    class IconDB;
    class IconHolder;
    class IconDbSequence;
    typedef boost::shared_ptr<IconHolder> CountedIconPtr;


    //methods used by worker thread
    void insertIntoBuffer(const DefaultChar* entryName, IconHolder& icon); //icon is invalidated by this call!!

//---------------------- Shared Data -------------------------
    std::auto_ptr<wxCriticalSection> lockIconDB;
    std::auto_ptr<IconDB> buffer;  //use synchronisation when accessing this!
    std::auto_ptr<IconDbSequence> bufSequence; //save sequence of buffer entry to delete oldest elements (implicitly shared by sharing Zstring with IconDB!!!)
//------------------------------------------------------------

    std::auto_ptr<WorkerThread> worker;
};
}

#endif // ICONBUFFER_H_INCLUDED
