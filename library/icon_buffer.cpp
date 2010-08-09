// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "icon_buffer.h"
#include <wx/msgdlg.h>
#include <map>
#include <queue>
#include <set>
#include <boost/thread.hpp>

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"

#elif defined FFS_LINUX
#include <giomm/file.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/main.h>
#endif


using ffs3::IconBuffer;


namespace
{
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
    static std::set<Zstring, LessFilename> exceptions;
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
    ::SHGetFileInfo(filename.c_str(), //ffs3::removeLongPathPrefix(fileName), //::SHGetFileInfo() can't handle \\?\-prefix!
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

    try //fallback: icon lookup may fail because some icons are currently not present on system
    {
        Glib::RefPtr<Gtk::IconTheme> iconTheme = Gtk::IconTheme::get_default();
        if (iconTheme)
        {
            Glib::RefPtr<Gdk::Pixbuf> iconPixbuf = iconTheme->load_icon("misc", ICON_SIZE, Gtk::ICON_LOOKUP_USE_BUILTIN);
            if (!iconPixbuf)
                iconPixbuf = iconTheme->load_icon("text-x-generic", ICON_SIZE, Gtk::ICON_LOOKUP_USE_BUILTIN);
            if (iconPixbuf)
                return IconHolder(iconPixbuf->gobj_copy()); //copy and pass icon ownership (may be 0)
        }
    }
    catch (const Glib::Error&) {}

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


class IconBuffer::WorkerThread
{
public:
    WorkerThread(IconBuffer& iconBuff);
    ~WorkerThread();

    void setWorkload(const std::vector<Zstring>& load); //(re-)set new workload of icons to be retrieved

    void operator()(); //thread entry

private:
    void doWork();

//---------------------- Shared Data -------------------------
    typedef BasicString FileName;

    struct SharedData
    {
        std::vector<FileName>     workload; //processes last elements of vector first!
        boost::mutex              mutex;
        boost::condition_variable condition; //signal event: data for processing available
    } shared;
//------------------------------------------------------------

    IconBuffer& iconBuffer;

    boost::thread threadObj;
};


IconBuffer::WorkerThread::WorkerThread(IconBuffer& iconBuff) :
    iconBuffer(iconBuff)
{
    threadObj = boost::thread(boost::ref(*this)); //localize all thread logic to this class!
}


IconBuffer::WorkerThread::~WorkerThread()
{
    setWorkload(std::vector<Zstring>()); //make sure interruption point is always reached!
    threadObj.interrupt();
    threadObj.join();
}


void IconBuffer::WorkerThread::setWorkload(const std::vector<Zstring>& load) //(re-)set new workload of icons to be retrieved
{
    {
        boost::lock_guard<boost::mutex> dummy(shared.mutex);

        shared.workload.clear();
        for (std::vector<Zstring>::const_iterator i = load.begin(); i != load.end(); ++i)
            shared.workload.push_back(FileName(i->c_str(), i->c_str() + i->length() + 1)); //make DEEP COPY from Zstring (include null-termination)!
    }

    shared.condition.notify_one();
    //condition handling, see: http://www.boost.org/doc/libs/1_43_0/doc/html/thread/synchronization.html#thread.synchronization.condvar_ref
}


void IconBuffer::WorkerThread::operator()() //thread entry
{
    try
    {
        while (true)
        {
            {
                boost::unique_lock<boost::mutex> dummy(shared.mutex);
                while(shared.workload.empty())
                    shared.condition.wait(dummy); //interruption point!
                //shared.condition.timed_wait(dummy, boost::get_system_time() + boost::posix_time::milliseconds(100));
            }

            doWork(); //no need to lock the complete method!
        }
    }
    catch (const std::exception& e) //exceptions must be catched per thread
    {
        wxMessageBox(wxString::FromAscii(e.what()), wxString(_("An exception occurred!")) + wxT("(Icon buffer)"), wxOK | wxICON_ERROR);
    }
}


void IconBuffer::WorkerThread::doWork()
{
    //do work: get the file icon.
    while (true)
    {
        Zstring fileName;
        {
            boost::lock_guard<boost::mutex> dummy(shared.mutex);
            if (shared.workload.empty())
                break; //enter waiting state
            fileName = &shared.workload.back()[0]; //deep copy (includes NULL-termination)
            shared.workload.pop_back();
        }

        if (iconBuffer.requestFileIcon(fileName)) //thread safety: Zstring okay, won't be reference-counted in requestIcon()
            continue; //icon already in buffer: skip

#ifdef FFS_WIN
        const Zstring extension = getFileExtension(fileName); //thread-safe: no sharing!
        if (isPriceyExtension(extension)) //"pricey" extensions are stored with fullnames and are read from disk, while cheap ones require just the extension
        {
            const IconHolder newIcon = IconBuffer::getAssociatedIcon(fileName);
            iconBuffer.insertIntoBuffer(fileName, newIcon);
        }
        else //no read-access to disk! determine icon by extension
        {
            const IconHolder newIcon = IconBuffer::getAssociatedIconByExt(extension);
            iconBuffer.insertIntoBuffer(extension, newIcon);
        }
#elif defined FFS_LINUX
        const IconHolder newIcon = IconBuffer::getAssociatedIcon(fileName);
        iconBuffer.insertIntoBuffer(fileName, newIcon);
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
    buffer(     new IconDB),
    bufSequence(new IconDbSequence),
    worker(     new WorkerThread(*this)) //might throw exceptions!
{}


IconBuffer::~IconBuffer() {} //auto_ptr<>: keep destructor non-inline


bool IconBuffer::requestFileIcon(const Zstring& fileName, wxIcon* icon)
{
    boost::lock_guard<boost::mutex> dummy(lockIconDB);

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
    boost::lock_guard<boost::mutex> dummy(lockIconDB);

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
