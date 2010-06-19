// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "iconBuffer.h"
#include <wx/thread.h>
#include <wx/bitmap.h>
#include <wx/msgdlg.h>
#include <wx/icon.h>
#include <map>
#include <queue>
#include <stdexcept>
#include <set>

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"

#elif defined FFS_LINUX
#include <giomm/file.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/main.h>
#endif


using FreeFileSync::IconBuffer;


namespace
{
struct CmpFilename
{
    bool operator()(const Zstring& a, const Zstring& b) const
    {
        return a.cmpFileName(b) < 0;
    }
};


#ifdef FFS_WIN
Zstring getFileExtension(const Zstring& filename)
{
    const Zstring shortName = filename.AfterLast(DefaultChar('\\')); //warning: using windows file name separator!
    const size_t pos = shortName.Find(DefaultChar('.'), true);
    return pos == Zstring::npos ?
           Zstring() :
           Zstring(shortName.c_str() + pos + 1);
}


//test for extension for icons that physically have to be retrieved from disc
bool isPriceyExtension(const Zstring& extension)
{
    static std::set<Zstring, CmpFilename> exceptions;
    static bool isInitalized = false;
    if (!isInitalized)
    {
        isInitalized = true;
        exceptions.insert(DefaultStr("exe"));
        exceptions.insert(DefaultStr("lnk"));
        exceptions.insert(DefaultStr("ico"));
        exceptions.insert(DefaultStr("ani"));
        exceptions.insert(DefaultStr("cur"));
        exceptions.insert(DefaultStr("url"));
        exceptions.insert(DefaultStr("msc"));
        exceptions.insert(DefaultStr("scr"));
    }
    return exceptions.find(extension) != exceptions.end();
}
#endif
}
//################################################################################################################################################


class IconBuffer::IconHolder //handle HICON/GdkPixbuf ownership WITHOUT ref-counting to allow thread-safe usage (in contrast to wxIcon)
{
public:
#ifdef FFS_WIN
    typedef HICON HandleType;
#elif defined FFS_LINUX
    typedef GdkPixbuf* HandleType;
#endif

    IconHolder(HandleType handle = 0) : handle_(handle) {} //take ownership!

    //icon holder has value semantics!
    IconHolder(const IconHolder& other) : handle_(other.handle_ == 0 ? 0 :
#ifdef FFS_WIN
                ::CopyIcon(other.handle_)
#elif defined FFS_LINUX
                gdk_pixbuf_copy(other.handle_) //create new Pix buf with reference count 1 or return 0 on error
#endif
                                                     ) {}

    IconHolder& operator=(const IconHolder& other)
    {
        IconHolder(other).swap(*this);
        return *this;
    }

    ~IconHolder()
    {
        if (handle_ != 0)
#ifdef FFS_WIN
            ::DestroyIcon(handle_);
#elif defined FFS_LINUX
            g_object_unref(handle_);
#endif
    }

    void swap(IconHolder& other) //throw()
    {
        std::swap(handle_, other.handle_);
    }

    wxIcon toWxIcon() const //copy HandleType, caller needs to take ownership!
    {
        IconHolder clone(*this);
        if (clone.handle_ != 0)
        {
            wxIcon newIcon; //attention: wxIcon uses reference counting!
#ifdef FFS_WIN
            newIcon.SetHICON(clone.handle_);  //
            newIcon.SetSize(IconBuffer::ICON_SIZE, IconBuffer::ICON_SIZE); //icon is actually scaled to this size (just in case referenced HICON differs)
#elif defined FFS_LINUX                       //
            newIcon.SetPixbuf(clone.handle_); // transfer ownership!!
#endif                                        //
            clone.handle_ = 0;                //
            return newIcon;
        }
        return wxNullIcon;
    }

private:
    HandleType handle_;
};


const wxIcon& IconBuffer::getDirectoryIcon() //one folder icon should be sufficient...
{
    static wxIcon folderIcon;

    static bool isInitalized = false;
    if (!isInitalized)
    {
        isInitalized = true;

#ifdef FFS_WIN
        SHFILEINFO fileInfo;
        fileInfo.hIcon = 0; //initialize hIcon

        //NOTE: CoInitializeEx()/CoUninitialize() implicitly called by wxWidgets on program startup!
        if (::SHGetFileInfo(DefaultStr("dummy"), //Windows Seven doesn't like this parameter to be an empty string
                            FILE_ATTRIBUTE_DIRECTORY,
                            &fileInfo,
                            sizeof(fileInfo),
                            SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES) &&

                fileInfo.hIcon != 0) //fix for weird error: SHGetFileInfo() might return successfully WITHOUT filling fileInfo.hIcon!!
        {
            folderIcon.SetHICON(fileInfo.hIcon); //transfer ownership!
            folderIcon.SetSize(IconBuffer::ICON_SIZE, IconBuffer::ICON_SIZE);
        }

#elif defined FFS_LINUX
        folderIcon = getAssociatedIcon(DefaultStr("/usr/")).toWxIcon(); //all directories will look like "/usr/"
#endif
    }
    return folderIcon;
}


IconBuffer::IconHolder IconBuffer::getAssociatedIcon(const Zstring& filename)
{
#ifdef FFS_WIN
    //despite what docu says about SHGetFileInfo() it can't handle all relative filenames, e.g. "\DirName"
    //but no problem, directory formatting takes care that filenames are always absolute!

    SHFILEINFO fileInfo;
    fileInfo.hIcon = 0; //initialize hIcon -> fix for weird error: SHGetFileInfo() might return successfully WITHOUT filling fileInfo.hIcon!!
    //bug report: https://sourceforge.net/tracker/?func=detail&aid=2768004&group_id=234430&atid=1093080

    //NOTE: CoInitializeEx()/CoUninitialize() implicitly called by wxWidgets on program startup!
    ::SHGetFileInfo(filename.c_str(), //FreeFileSync::removeLongPathPrefix(fileName), //::SHGetFileInfo() can't handle \\?\-prefix!
                    0,
                    &fileInfo,
                    sizeof(fileInfo),
                    SHGFI_ICON | SHGFI_SMALLICON);

    return IconHolder(fileInfo.hIcon); //pass icon ownership (may be 0)

#elif defined FFS_LINUX
    static struct RunOnce
    {
        RunOnce()
        {
            Gtk::Main::init_gtkmm_internals();
        }
    } dummy;

    try
    {
        Glib::RefPtr<Gio::File> fileObj = Gio::File::create_for_path(filename.c_str()); //never fails
        Glib::RefPtr<Gio::FileInfo> fileInfo = fileObj->query_info(G_FILE_ATTRIBUTE_STANDARD_ICON);
        if (fileInfo)
        {
            Glib::RefPtr<Gio::Icon> gicon = fileInfo->get_icon();
            if (gicon)
            {
                Glib::RefPtr<Gtk::IconTheme> iconTheme = Gtk::IconTheme::get_default();
                if (iconTheme)
                {
                    Gtk::IconInfo iconInfo = iconTheme->lookup_icon(gicon, ICON_SIZE, Gtk::ICON_LOOKUP_USE_BUILTIN); //this may fail if icon is not installed on system
                    if (iconInfo)
                    {
                        Glib::RefPtr<Gdk::Pixbuf> iconPixbuf = iconInfo.load_icon(); //render icon into Pixbuf
                        if (iconPixbuf)
                            return IconHolder(iconPixbuf->gobj_copy()); //copy and pass icon ownership (may be 0)
                    }
                }
            }
        }
    }
    catch (const Glib::Error&) {}


    //fallback: icon lookup may fail because some icons are currently not present on system
    Glib::RefPtr<Gtk::IconTheme> iconTheme = Gtk::IconTheme::get_default();
    if (iconTheme)
    {
        Glib::RefPtr<Gdk::Pixbuf> iconPixbuf = iconTheme->load_icon("misc", ICON_SIZE, Gtk::ICON_LOOKUP_USE_BUILTIN);
        if (!iconPixbuf)
            iconPixbuf = iconTheme->load_icon("text-x-generic", ICON_SIZE, Gtk::ICON_LOOKUP_USE_BUILTIN);
        if (iconPixbuf)
            return IconHolder(iconPixbuf->gobj_copy()); //copy and pass icon ownership (may be 0)
    }

    //fallback fallback
    return IconHolder();
#endif
}


#ifdef FFS_WIN
IconBuffer::IconHolder IconBuffer::getAssociatedIconByExt(const Zstring& extension)
{
    SHFILEINFO fileInfo;
    fileInfo.hIcon = 0; //initialize hIcon -> fix for weird error: SHGetFileInfo() might return successfully WITHOUT filling fileInfo.hIcon!!

    //no read-access to disk! determine icon by extension
    ::SHGetFileInfo((Zstring(DefaultStr("dummy.")) + extension).c_str(),  //Windows Seven doesn't like this parameter to be without short name
                    FILE_ATTRIBUTE_NORMAL,
                    &fileInfo,
                    sizeof(fileInfo),
                    SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);

    return IconHolder(fileInfo.hIcon); //pass icon ownership (may be 0)
}
#endif


//---------------------------------------------------------------------------------------------------
typedef std::vector<DefaultChar> BasicString; //simple thread safe string class: std::vector is guaranteed to not use reference counting, Effective STL, item 13
//avoid reference-counted objects as shared data: NOT THREADSAFE!!! (implicitly shared variables: ref-count + c-string)
//---------------------------------------------------------------------------------------------------


class IconBuffer::WorkerThread : public wxThread
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
//------------------------------------------------------------

    //signal event: icon buffer(main thread) -> worker thread
    wxMutex threadIsListening;
    wxCondition continueWork; //wake up thread

    IconBuffer* iconBuffer;
};


IconBuffer::WorkerThread::WorkerThread(IconBuffer* iconBuff) :
    wxThread(wxTHREAD_DETACHED), //we're using the thread encapsulated in a static object => use "detached" to avoid main thread waiting for this thread on exit(which in turn would prevent deletion of static object...ect.) => deadlock!
    threadHasMutex(false),
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


void IconBuffer::WorkerThread::setWorkload(const std::vector<Zstring>& load) //(re-)set new workload of icons to be retrieved
{
    wxCriticalSectionLocker dummy(lockWorkload);

    workload.clear();
    for (std::vector<Zstring>::const_iterator i = load.begin(); i != load.end(); ++i)
        workload.push_back(FileName(i->c_str(), i->c_str() + i->length() + 1)); //make DEEP COPY from Zstring (include null-termination)!

    if (!workload.empty())
        continueWork.Signal(); //wake thread IF he is waiting
}


void IconBuffer::WorkerThread::quitThread()
{
    setWorkload(std::vector<Zstring>());
    Delete(); //gracefully terminate a detached thread...
}


wxThread::ExitCode IconBuffer::WorkerThread::Entry()
{
    try
    {
        //this lock needs to be called from WITHIN the thread => calling it from constructor(Main thread) would be useless (signal direction: main -> thread)
        wxMutexLocker dummy(threadIsListening); //this mutex STAYS locked all the time except of continueWork.Wait()!
        {
            wxCriticalSectionLocker dummy2(lockWorkload);
            threadHasMutex = true;
        }

        while (true)
        {
            continueWork.WaitTimeout(100); //waiting for continueWork.Signal(); unlocks Mutex "threadIsListening"

            if (TestDestroy())
                return 0;

            doWork();
        }
    }
    catch (const std::exception& e) //exceptions must be catched per thread
    {
        wxMessageBox(wxString::FromAscii(e.what()), _("An exception occured!"), wxOK | wxICON_ERROR);
        return 0;
    }
}


void IconBuffer::WorkerThread::doWork()
{
    //do work: get the file icon.
    while (true)
    {
        Zstring fileName;
        {
            wxCriticalSectionLocker dummy(lockWorkload);
            if (workload.empty())
                break; //enter waiting state
            fileName = &workload.back()[0]; //deep copy (includes NULL-termination)
            workload.pop_back();
        }

        if (iconBuffer->requestFileIcon(fileName)) //thread safety: Zstring okay, won't be reference-counted in requestIcon()
            continue; //icon already in buffer: skip

#ifdef FFS_WIN
        const Zstring extension = getFileExtension(fileName); //thread-safe: no sharing!
        if (isPriceyExtension(extension)) //"pricey" extensions are stored with fullnames and are read from disk, while cheap ones require just the extension
        {
            const IconHolder newIcon = IconBuffer::getAssociatedIcon(fileName);
            iconBuffer->insertIntoBuffer(fileName, newIcon);
        }
        else //no read-access to disk! determine icon by extension
        {
            const IconHolder newIcon = IconBuffer::getAssociatedIconByExt(extension);
            iconBuffer->insertIntoBuffer(extension, newIcon);
        }
#elif defined FFS_LINUX
        const IconHolder newIcon = IconBuffer::getAssociatedIcon(fileName);
        iconBuffer->insertIntoBuffer(fileName, newIcon);
#endif
    }
}


//---------------------------------------------------------------------------------------------------
class IconBuffer::IconDB : public std::map<Zstring, IconBuffer::IconHolder> {}; //entryName/icon -> ATTENTION: avoid ref-counting for this shared data structure!!! (== don't copy instances between threads)
class IconBuffer::IconDbSequence : public std::queue<Zstring> {}; //entryName
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


bool IconBuffer::requestFileIcon(const Zstring& fileName, wxIcon* icon)
{
    wxCriticalSectionLocker dummy(*lockIconDB);

#ifdef FFS_WIN
    //"pricey" extensions are stored with fullnames and are read from disk, while cheap ones require just the extension
    const Zstring extension = getFileExtension(fileName);
    IconDB::const_iterator i = buffer->find(isPriceyExtension(extension) ? fileName : extension);
#elif defined FFS_LINUX
    IconDB::const_iterator i = buffer->find(fileName);
#endif

    if (i == buffer->end())
        return false;

    if (icon != NULL)
        *icon = i->second.toWxIcon();

    return true;
}


void IconBuffer::setWorkload(const std::vector<Zstring>& load)
{
    worker->setWorkload(load);
}


void IconBuffer::insertIntoBuffer(const DefaultChar* entryName, const IconHolder& icon) //called by worker thread
{
    wxCriticalSectionLocker dummy(*lockIconDB);

    //thread safety, ref-counting: (implicitly) make deep copy!
    const Zstring fileNameZ = entryName;

    const std::pair<IconDB::iterator, bool> rc = buffer->insert(std::make_pair(fileNameZ, icon)); //thread saftey: icon uses ref-counting! But is NOT shared with main thread!

    if (rc.second) //if insertion took place
        bufSequence->push(fileNameZ); //note: sharing Zstring with IconDB!!!

    assert(buffer->size() == bufSequence->size());

    //remove elements if buffer becomes too big:
    if (buffer->size() > BUFFER_SIZE) //limit buffer size: critical because GDI resources are limited (e.g. 10000 on XP per process)
    {
        //remove oldest element
        buffer->erase(bufSequence->front());
        bufSequence->pop();
    }
}
