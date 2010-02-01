#ifndef ICONBUFFER_H_INCLUDED
#define ICONBUFFER_H_INCLUDED

#ifndef FFS_WIN
header should be used in the windows build only!
#endif

#include <vector>
#include "../shared/zstring.h"
#include <memory>

class wxCriticalSection;
class WorkerThread;
class IconDB;
class IconDbSequence;
class wxIcon;


namespace FreeFileSync
{
class IconBuffer
{
    friend class ::WorkerThread;

public:
    static IconBuffer& getInstance();

    static const wxIcon& getDirectoryIcon(); //one folder icon should be sufficient...

    bool requestFileIcon(const Zstring& fileName, wxIcon* icon = NULL); //returns false if icon is not in buffer
    void setWorkload(const std::vector<Zstring>& load); //(re-)set new workload of icons to be retrieved;

    static const int    ICON_SIZE   = 16;  //size in pixel
    static const size_t BUFFER_SIZE = 800; //maximum number if icons to buffer

private:
    IconBuffer();
    ~IconBuffer();

    //methods used by worker thread
    void insertIntoBuffer(const DefaultChar* entryName, const wxIcon& icon);

//---------------------- Shared Data -------------------------
    std::auto_ptr<wxCriticalSection> lockIconDB;
    std::auto_ptr<IconDB> buffer;  //use synchronisation when accessing this!
//------------------------------------------------------------

    std::auto_ptr<IconDbSequence> bufSequence; //save sequence of buffer entry to delte olderst elements

    std::auto_ptr<WorkerThread> worker;
};
}

#endif // ICONBUFFER_H_INCLUDED
