// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef ICONBUFFER_H_INCLUDED
#define ICONBUFFER_H_INCLUDED

#include <vector>
#include "../shared/zstring.h"
#include <memory>
#include <wx/icon.h>
#include "../shared/boost_thread_wrap.h" //include <boost/thread.hpp>


namespace ffs3
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

    static const size_t BUFFER_SIZE_MAX = 800; //maximum number of icons to buffer

    class IconDB;
    class IconHolder;
    class IconDbSequence;

//---------------------------------------------------------------------------------------------------
    typedef Zbase<Zchar, StorageDeepCopy> BasicString; //thread safe string class
//avoid reference-counted objects for shared data: NOT THREADSAFE!!! (implicitly shared variable: ref-count)
//---------------------------------------------------------------------------------------------------

    //methods used by worker thread
    void insertIntoBuffer(const BasicString& entryName, const IconHolder& icon);

    static IconHolder getAssociatedIcon(const BasicString& filename);
    static IconHolder getAssociatedIconByExt(const BasicString& extension);

#ifdef FFS_WIN
    static BasicString getFileExtension(const BasicString& filename);
    static bool isPriceyExtension(const BasicString& extension);
#endif

//---------------------- Shared Data -------------------------
    boost::mutex lockIconDB;
    std::auto_ptr<IconDB> buffer;  //use synchronisation when accessing this!
    std::auto_ptr<IconDbSequence> bufSequence; //save sequence of buffer entry to delete oldest elements
//------------------------------------------------------------

    class WorkerThread;
    std::auto_ptr<WorkerThread> worker;
};
}

#endif // ICONBUFFER_H_INCLUDED
