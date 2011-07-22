// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "icon_buffer.h"
#include <wx/msgdlg.h>
#include <map>
#include <vector>
#include <queue>
#include <set>
#include <wx/log.h>
#include "../shared/i18n.h"
#include "../shared/boost_thread_wrap.h" //include <boost/thread.hpp>

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"

#elif defined FFS_LINUX
#include <giomm/file.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/main.h>
#endif


using namespace zen;

const size_t BUFFER_SIZE_MAX = 800; //maximum number of icons to buffer

//---------------------------------------------------------------------------------------------------
typedef Zbase<Zchar, StorageDeepCopy> BasicString; //thread safe string class
//avoid reference-counted objects for shared data: NOT THREADSAFE!!! (implicitly shared variable: ref-count)
//---------------------------------------------------------------------------------------------------


#ifdef FFS_WIN
BasicString getFileExtension(const BasicString& filename)
{
    const BasicString shortName = filename.AfterLast(Zchar('\\')); //warning: using windows file name separator!

    return shortName.find(Zchar('.')) != BasicString::npos ?
           filename.AfterLast(Zchar('.')) :
           BasicString();
}


//test for extension for icons that physically have to be retrieved from disc
bool isPriceyExtension(const BasicString& extension)
{
    static std::set<BasicString, LessFilename> exceptions; //not thread-safe, but called from worker thread only!
    if (exceptions.empty())
    {
        exceptions.insert(Zstr("exe"));
        exceptions.insert(Zstr("lnk"));
        exceptions.insert(Zstr("ico"));
        exceptions.insert(Zstr("ani"));
        exceptions.insert(Zstr("cur"));
        exceptions.insert(Zstr("url"));
        exceptions.insert(Zstr("msc"));
        exceptions.insert(Zstr("scr"));
    }
    return exceptions.find(extension) != exceptions.end();
}
#endif


//################################################################################################################################################
class IconHolder //handle HICON/GdkPixbuf ownership WITHOUT ref-counting to allow thread-safe usage (in contrast to wxIcon)
{
public:
#ifdef FFS_WIN
    typedef HICON HandleType;
#elif defined FFS_LINUX
    typedef GdkPixbuf* HandleType;
#endif

    IconHolder(HandleType handle = 0) : handle_(handle) {} //take ownership!

    //icon holder has value semantics!
    IconHolder(const IconHolder& other) : handle_(other.handle_ == NULL ? NULL :
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
        if (handle_ != NULL)
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
        if (handle_ == NULL)
            return wxNullIcon;

        IconHolder clone(*this);

        wxIcon newIcon; //attention: wxIcon uses reference counting!
#ifdef FFS_WIN
        newIcon.SetHICON(clone.handle_);  //
        newIcon.SetSize(IconBuffer::ICON_SIZE, IconBuffer::ICON_SIZE); //icon is actually scaled to this size (just in case referenced HICON differs)
#elif defined FFS_LINUX                   //
        newIcon.SetPixbuf(clone.handle_); // transfer ownership!!
#endif                                    //
        clone.handle_ = NULL;             //
        return newIcon;
    }

private:
    HandleType handle_;
};


IconHolder getAssociatedIcon(const BasicString& filename)
{
#ifdef FFS_WIN
    //despite what docu says about SHGetFileInfo() it can't handle all relative filenames, e.g. "\DirName"
    //but no problem, directory formatting takes care that filenames are always absolute!

    SHFILEINFO fileInfo = {}; //initialize hIcon -> fix for weird error: SHGetFileInfo() might return successfully WITHOUT filling fileInfo.hIcon!!
    //bug report: https://sourceforge.net/tracker/?func=detail&aid=2768004&group_id=234430&atid=1093080

    //NOTE: CoInitializeEx()/CoUninitialize() needs to be called for THIS thread!
    ::SHGetFileInfo(filename.c_str(), //zen::removeLongPathPrefix(fileName), //::SHGetFileInfo() can't handle \\?\-prefix!
                    0,
                    &fileInfo,
                    sizeof(fileInfo),
                    SHGFI_ICON | SHGFI_SMALLICON);

    return IconHolder(fileInfo.hIcon); //pass icon ownership (may be 0)

#elif defined FFS_LINUX
    //call Gtk::Main::init_gtkmm_internals() on application startup!!
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
                    Gtk::IconInfo iconInfo = iconTheme->lookup_icon(gicon, IconBuffer::ICON_SIZE, Gtk::ICON_LOOKUP_USE_BUILTIN); //this may fail if icon is not installed on system
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
            Glib::RefPtr<Gdk::Pixbuf> iconPixbuf = iconTheme->load_icon("misc", IconBuffer::ICON_SIZE, Gtk::ICON_LOOKUP_USE_BUILTIN);
            if (!iconPixbuf)
                iconPixbuf = iconTheme->load_icon("text-x-generic", IconBuffer::ICON_SIZE, Gtk::ICON_LOOKUP_USE_BUILTIN);
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
IconHolder getAssociatedIconByExt(const BasicString& extension)
{
    SHFILEINFO fileInfo = {}; //initialize hIcon -> fix for weird error: SHGetFileInfo() might return successfully WITHOUT filling fileInfo.hIcon!!

    //no read-access to disk! determine icon by extension
    ::SHGetFileInfo((Zstr("dummy.") + extension).c_str(),  //Windows Seven doesn't like this parameter to be without short name
                    FILE_ATTRIBUTE_NORMAL,
                    &fileInfo,
                    sizeof(fileInfo),
                    SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);

    return IconHolder(fileInfo.hIcon); //pass icon ownership (may be 0)
}
#endif


const wxIcon& getDirectoryIcon()
{
    static wxIcon folderIcon;

    static bool isInitalized = false; //not thread-safe, but called from GUI thread only!
    if (!isInitalized)
    {
        isInitalized = true;

#ifdef FFS_WIN
        SHFILEINFO fileInfo = {}; //initialize hIcon

        //NOTE: CoInitializeEx()/CoUninitialize() needs to be called for THIS thread!
        if (::SHGetFileInfo(Zstr("dummy"), //Windows Seven doesn't like this parameter to be an empty string
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
        folderIcon = ::getAssociatedIcon(Zstr("/usr/")).toWxIcon(); //all directories will look like "/usr/"
#endif
    }
    return folderIcon;
}


const wxIcon& getFileIcon()
{
    static wxIcon fileIcon;

    static bool isInitalized = false; //not thread-safe, but called from GUI thread only!
    if (!isInitalized)
    {
        isInitalized = true;

#ifdef FFS_WIN
        SHFILEINFO fileInfo = {};  //initialize hIcon

        //NOTE: CoInitializeEx()/CoUninitialize() needs to be called for THIS thread!
        if (::SHGetFileInfo(Zstr("dummy"), //Windows Seven doesn't like this parameter to be an empty string
                            FILE_ATTRIBUTE_NORMAL,
                            &fileInfo,
                            sizeof(fileInfo),
                            SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES) &&

            fileInfo.hIcon != 0) //fix for weird error: SHGetFileInfo() might return successfully WITHOUT filling fileInfo.hIcon!!
        {
            fileIcon.SetHICON(fileInfo.hIcon); //transfer ownership!
            fileIcon.SetSize(IconBuffer::ICON_SIZE, IconBuffer::ICON_SIZE);
        }

#elif defined FFS_LINUX
        try
        {
            Glib::RefPtr<Gtk::IconTheme> iconTheme = Gtk::IconTheme::get_default();
            if (iconTheme)
            {
                Glib::RefPtr<Gdk::Pixbuf> iconPixbuf = iconTheme->load_icon("misc", IconBuffer::ICON_SIZE, Gtk::ICON_LOOKUP_USE_BUILTIN);
                if (!iconPixbuf)
                    iconPixbuf = iconTheme->load_icon("text-x-generic", IconBuffer::ICON_SIZE, Gtk::ICON_LOOKUP_USE_BUILTIN);
                if (iconPixbuf)
                    fileIcon.SetPixbuf(iconPixbuf->gobj_copy()); // transfer ownership!!
            }
        }
        catch (const Glib::Error&) {}
#endif
    }
    return fileIcon;
}


//################################################################################################################################################
//---------------------- Shared Data -------------------------
struct WorkLoad
{
    std::vector<BasicString>  filesToLoad; //processes last elements of vector first!
    boost::mutex              mutex;
    boost::condition_variable condition; //signal event: data for processing available
};

typedef std::map<BasicString, IconHolder, LessFilename> NameIconMap; //entryName/icon -> ATTENTION: avoid ref-counting for this shared data structure!
typedef std::queue<BasicString> IconDbSequence; //entryName

struct Buffer
{
    boost::mutex   lockAccess;
    NameIconMap    iconMappping;   //use synchronisation when accessing this!
    IconDbSequence iconSequence; //save sequence of buffer entry to delete oldest elements
};
//------------------------------------------------------------


bool requestFileIcon(Buffer& buf, const Zstring& fileName, wxIcon* icon = NULL)
{
    boost::lock_guard<boost::mutex> dummy(buf.lockAccess);

#ifdef FFS_WIN
    //"pricey" extensions are stored with fullnames and are read from disk, while cheap ones require just the extension
    const BasicString extension    = getFileExtension(BasicString(fileName));
    const BasicString searchString = isPriceyExtension(extension) ? BasicString(fileName) : extension;
    auto iter = buf.iconMappping.find(searchString);
#elif defined FFS_LINUX
    auto iter = buf.iconMappping.find(BasicString(fileName));
#endif

    if (iter == buf.iconMappping.end())
        return false;

    if (icon != NULL)
        *icon = iter->second.toWxIcon();
    return true;
}


void insertIntoBuffer(Buffer& buf, const BasicString& entryName, const IconHolder& icon) //called by worker thread
{
    boost::lock_guard<boost::mutex> dummy(buf.lockAccess);

    //thread saftey: icon uses ref-counting! But is NOT shared with main thread!
    auto rc = buf.iconMappping.insert(std::make_pair(entryName, icon));
    if (rc.second) //if insertion took place
        buf.iconSequence.push(entryName); //note: sharing Zstring with IconDB!!!

    assert(buf.iconMappping.size() == buf.iconSequence.size());

    //remove elements if buffer becomes too big:
    if (buf.iconMappping.size() > BUFFER_SIZE_MAX) //limit buffer size: critical because GDI resources are limited (e.g. 10000 on XP per process)
    {
        //remove oldest element
        buf.iconMappping.erase(buf.iconSequence.front());
        buf.iconSequence.pop();
    }
}
//################################################################################################################################################

class WorkerThread //lifetime is part of icon buffer
{
public:
    WorkerThread(const std::shared_ptr<WorkLoad>& workload,
                 const std::shared_ptr<Buffer>& buffer) :
        workload_(workload),
        buffer_(buffer) {}

    void operator()(); //thread entry

private:
    void doWork();

    std::shared_ptr<WorkLoad> workload_;
    std::shared_ptr<Buffer> buffer_;
};


void WorkerThread::operator()() //thread entry
{
    //failure to initialize COM for each thread is a source of hard to reproduce bugs: https://sourceforge.net/tracker/?func=detail&aid=3160472&group_id=234430&atid=1093080
#ifdef FFS_WIN
    struct ThreadInitializer
    {
        ThreadInitializer () { ::CoInitializeEx(NULL, COINIT_MULTITHREADED); }
        ~ThreadInitializer() { ::CoUninitialize(); }
    } dummy1;
#endif

    try
    {
        while (true)
        {
            {
                boost::unique_lock<boost::mutex> dummy(workload_->mutex);
                while(workload_->filesToLoad.empty())
                    workload_->condition.wait(dummy); //interruption point!
                //shared.condition.timed_wait(dummy, boost::get_system_time() + boost::posix_time::milliseconds(100));
            }

            doWork(); //no need to lock the complete method!
        }
    }
    catch (boost::thread_interrupted&)
    {
        throw; //this is the only reasonable exception!
    }
    catch (const std::exception& e) //exceptions must be catched per thread
    {
        wxSafeShowMessage(wxString(_("An exception occurred!")) + wxT("(Icon buffer)"), wxString::FromAscii(e.what())); //simple wxMessageBox won't do for threads
    }
    catch (...) //exceptions must be catched per thread
    {
        wxSafeShowMessage(wxString(_("An exception occurred!")) + wxT("(Icon buffer2)"), wxT("Unknown exception in icon thread!")); //simple wxMessageBox won't do for threads
    }
}


void WorkerThread::doWork()
{
    //do work: get the file icon.
    while (true)
    {
        BasicString fileName;
        {
            boost::lock_guard<boost::mutex> dummy(workload_->mutex);
            if (workload_->filesToLoad.empty())
                break; //enter waiting state
            fileName = workload_->filesToLoad.back(); //deep copy
            workload_->filesToLoad.pop_back();
        }

        if (requestFileIcon(*buffer_, Zstring(fileName))) //thread safety: Zstring okay, won't be reference-counted in requestIcon()
            continue; //icon already in buffer: skip

#ifdef FFS_WIN
        const BasicString extension = getFileExtension(fileName); //thread-safe: no sharing!
        if (isPriceyExtension(extension)) //"pricey" extensions are stored with fullnames and are read from disk, while cheap ones require just the extension
        {
            const IconHolder newIcon = getAssociatedIcon(fileName);
            insertIntoBuffer(*buffer_, fileName, newIcon);
        }
        else //no read-access to disk! determine icon by extension
        {
            const IconHolder newIcon = getAssociatedIconByExt(extension);
            insertIntoBuffer(*buffer_, extension, newIcon);
        }
#elif defined FFS_LINUX
        const IconHolder newIcon = getAssociatedIcon(fileName);
        insertIntoBuffer(*buffer_, fileName, newIcon);
#endif
    }
}


//#########################  redirect to impl  #####################################################

struct IconBuffer::Pimpl
{
    Pimpl() :
        workload(std::make_shared<WorkLoad>()),
        buffer(std::make_shared<Buffer>()) {}


    std::shared_ptr<WorkLoad> workload;
    std::shared_ptr<Buffer> buffer;

    boost::thread worker;
};


IconBuffer::IconBuffer() : pimpl(new Pimpl)
{
    pimpl->worker = boost::thread(WorkerThread(pimpl->workload, pimpl->buffer));
}


IconBuffer::~IconBuffer()
{
    setWorkload(std::vector<Zstring>()); //make sure interruption point is always reached!
    pimpl->worker.interrupt();
    pimpl->worker.join();
}

const wxIcon& IconBuffer::getDirectoryIcon() { return ::getDirectoryIcon(); }

const wxIcon& IconBuffer::getFileIcon() { return ::getFileIcon(); }

IconBuffer& IconBuffer::getInstance()
{
    static IconBuffer instance;
    return instance;
}

bool IconBuffer::requestFileIcon(const Zstring& fileName, wxIcon* icon) { return ::requestFileIcon(*pimpl->buffer, fileName, icon); }

void IconBuffer::setWorkload(const std::vector<Zstring>& load)
{
    {
        boost::lock_guard<boost::mutex> dummy(pimpl->workload->mutex);

        pimpl->workload->filesToLoad.clear();

        std::transform(load.begin(), load.end(), std::back_inserter(pimpl->workload->filesToLoad),
        [](const Zstring& file) { return BasicString(file); }); //make DEEP COPY from Zstring
    }

    pimpl->workload->condition.notify_one();
    //condition handling, see: http://www.boost.org/doc/libs/1_43_0/doc/html/thread/synchronization.html#thread.synchronization.condvar_ref
}
