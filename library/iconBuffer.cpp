#include "iconBuffer.h"
#include <wx/thread.h>
#include <wx/bitmap.h>
#include <wx/msw/wrapwin.h> //includes "windows.h"
#include <wx/msgdlg.h>
#include <wx/icon.h>
#include <map>
#include <queue>
#include <stdexcept>

using FreeFileSync::IconBuffer;


typedef std::vector<DefaultChar> BasicString; //simple thread safe string class: std::vector is guaranteed to not use reference counting, Effective STL, item 13


class WorkerThread : public wxThread
{
public:
    WorkerThread(IconBuffer* iconBuff);

    void setWorkload(const std::vector<Zstring>& load); //(re-)set new workload of icons to be retrieved
    void quitThread();

    virtual ExitCode Entry();

private:
    void doWork();

//---------------------- Shared Data -------------------------
    typedef BasicString FileName;

    wxCriticalSection lockWorkload; //use for locking shared data
    std::vector<FileName> workload; //processes last elements of vector first!
    bool threadHasMutex;
    bool threadExitIsRequested;
//------------------------------------------------------------

    //event: icon buffer -> woker thread
    wxMutex threadIsListening;
    wxCondition continueWork; //wake up thread

    IconBuffer* iconBuffer;
};


WorkerThread::WorkerThread(IconBuffer* iconBuff) :
        wxThread(wxTHREAD_JOINABLE),
        threadHasMutex(false),
        threadExitIsRequested(false),
        threadIsListening(),
        continueWork(threadIsListening),
        iconBuffer(iconBuff)
{
    if (Create() != wxTHREAD_NO_ERROR)
        throw std::runtime_error("Error creating icon buffer worker thread!");

    if (Run() != wxTHREAD_NO_ERROR)
        throw std::runtime_error("Error starting icon buffer worker thread!");

    //wait until thread has aquired mutex
    bool hasMutex = false;
    while (!hasMutex)
    {
        wxMilliSleep(5);
        wxCriticalSectionLocker dummy(lockWorkload);
        hasMutex = threadHasMutex;
    }   //polling -> it's not nice, but works and is no issue
}


void WorkerThread::setWorkload(const std::vector<Zstring>& load) //(re-)set new workload of icons to be retrieved
{
    wxCriticalSectionLocker dummy(lockWorkload);

    workload.clear();
    for (std::vector<Zstring>::const_iterator i = load.begin(); i != load.end(); ++i)
        workload.push_back(FileName(i->c_str(), i->c_str() + i->length() + 1)); //make DEEP COPY from Zstring (include null-termination)!

    if (!workload.empty())
        continueWork.Signal(); //wake thread IF he is waiting
}


void WorkerThread::quitThread()
{
    {
        wxMutexLocker dummy(threadIsListening); //wait until thread is in waiting state
        threadExitIsRequested = true; //no sharing conflicts in this situation
        continueWork.Signal(); //exit thread
    }
    Wait(); //wait until thread has exitted
}


wxThread::ExitCode WorkerThread::Entry()
{
    try
    {
        wxMutexLocker dummy(threadIsListening); //this lock needs to be called from WITHIN the thread => calling it from constructor(Main thread) would be useless
        {                                       //this mutex STAYS locked all the time except of continueWork.Wait()!
            wxCriticalSectionLocker dummy2(lockWorkload);
            threadHasMutex = true;
        }

        while (true)
        {
            continueWork.Wait(); //waiting for continueWork.Signal(); unlocks Mutex "threadIsListening"

            //no mutex needed in this context
            if (threadExitIsRequested) //no mutex here: atomicity is not prob for a bool, but visibility (e.g. caching in registers)
                return 0;              //shouldn't be a problem nevertheless because of implicit memory barrier caused by mutex.Lock() in .Wait()

            //do work: get the file icon.
            doWork();
        }
    }
    catch (const std::exception& e) //exceptions must be catched per thread
    {
        wxMessageBox(wxString::From8BitData(e.what()), _("An exception occured!"), wxOK | wxICON_ERROR);
        return 0;
    }
}


void WorkerThread::doWork()
{
    FileName fileName; //don't use Zstring: reference-counted objects are NOT THREADSAFE!!! e.g. double deletion might happen

    //do work: get the file icon.
    while (true)
    {
        {
            wxCriticalSectionLocker dummy(lockWorkload);
            if (workload.empty())
                break; //enter waiting state
            fileName = workload.back();
            workload.pop_back();
        }

        if (iconBuffer->requestIcon(Zstring(&fileName[0]))) //thread safety: Zstring okay, won't be reference-counted in requestIcon(), fileName is NOT empty
            break; //icon already in buffer: enter waiting state

        //despite what docu says about SHGetFileInfo() it can't handle all relative filenames, e.g. "\DirName"
        const unsigned int BUFFER_SIZE = 10000;
        DefaultChar fullName[BUFFER_SIZE];
        const DWORD rv = ::GetFullPathName(
                             &fileName[0], //__in   LPCTSTR lpFileName,
                             BUFFER_SIZE,  //__in   DWORD nBufferLength,
                             fullName,     //__out  LPTSTR lpBuffer,
                             NULL);        //__out  LPTSTR *lpFilePart
        if (rv < BUFFER_SIZE && rv != 0)
        {
            //load icon
            SHFILEINFO fileInfo;
            fileInfo.hIcon = 0; //initialize hIcon

            if (::SHGetFileInfo(fullName, //NOTE: CoInitializeEx()/CoUninitialize() implicitly called by wxWidgets on program startup!
                                0,
                                &fileInfo,
                                sizeof(fileInfo),
                                SHGFI_ICON | SHGFI_SMALLICON) &&

                    fileInfo.hIcon != 0) //fix for weird error: SHGetFileInfo() might return successfully WITHOUT filling fileInfo.hIcon!!
            {                            //bug report: https://sourceforge.net/tracker/?func=detail&aid=2768004&group_id=234430&atid=1093080

                wxIcon newIcon; //attention: wxIcon uses reference counting!
                newIcon.SetHICON(fileInfo.hIcon);
                newIcon.SetSize(IconBuffer::ICON_SIZE, IconBuffer::ICON_SIZE);

                iconBuffer->insertIntoBuffer(&fileName[0], newIcon); //thread safety: icon may be deleted only within insertIntoBuffer()

                //freeing of icon handle seems to happen somewhere beyond wxIcon destructor
                //if (!DestroyIcon(fileInfo.hIcon))
                //  throw RuntimeException(wxString(wxT("Error deallocating Icon handle!\n\n")) + FreeFileSync::getLastErrorFormatted());
                continue;
            }
        }
        //if loading of icon fails for whatever reason, just save a dummy icon to avoid re-loading
        iconBuffer->insertIntoBuffer(&fileName[0], wxNullIcon);
    }
}

//---------------------------------------------------------------------------------------------------

typedef Zstring FileName;
class IconDB : public std::map<FileName, wxIcon> {};
class IconDbSequence : public std::queue<FileName> {};

//---------------------------------------------------------------------------------------------------


IconBuffer& IconBuffer::getInstance()
{
    static IconBuffer instance;
    return instance;
}


IconBuffer::IconBuffer() :
        lockIconDB( new wxCriticalSection),
        buffer(     new IconDB),
        bufSequence(new IconDbSequence),
        worker(     new WorkerThread(this)) //might throw exceptions!
{}


IconBuffer::~IconBuffer()
{
    worker->quitThread();
}


bool IconBuffer::requestIcon(const Zstring& fileName, wxIcon* icon)
{
    wxCriticalSectionLocker dummy(*lockIconDB);
    IconDB::const_iterator i = buffer->find(fileName);

    if (i != buffer->end())
    {
        if (icon != NULL)
            *icon = i->second;
        return true;
    }

    return false;
}


void IconBuffer::setWorkload(const std::vector<Zstring>& load)
{
    worker->setWorkload(load);
}


void IconBuffer::insertIntoBuffer(const DefaultChar* fileName, const wxIcon& icon) //called by worker thread
{
    if (icon.IsOk()) //this check won't hurt
    {
        wxCriticalSectionLocker dummy(*lockIconDB);

        const Zstring fileNameZ = fileName;

        const std::pair<IconDB::iterator, bool> rc = buffer->insert(IconDB::value_type(fileNameZ, icon));

        if (rc.second) //if insertion took place
            bufSequence->push(fileNameZ);

        assert(buffer->size() == bufSequence->size());

        //remove elements if buffer becomes too big:
        if (buffer->size() > 1000) //limit buffer size: critical because large buffers seem to cause various wxIcon/wxBitmap issues!
        {
            //remove oldest element
            buffer->erase(bufSequence->front());
            bufSequence->pop();
        }
    }
}

