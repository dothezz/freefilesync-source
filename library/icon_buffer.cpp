// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "icon_buffer.h"
#include <wx/msgdlg.h>
#include <map>
#include <vector>
#include <queue>
#include <set>
#include <wx/log.h>
#include "../shared/i18n.h"
#include "../shared/boost_thread_wrap.h" //include <boost/thread.hpp>
#include "../shared/loki/ScopeGuard.h"
#include <boost/thread/once.hpp>

#ifdef FFS_WIN
#include <wx/msw/wrapwin.h> //includes "windows.h"

#elif defined FFS_LINUX
#include <giomm/file.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/main.h>
#endif


using namespace zen;

const size_t BUFFER_SIZE_MAX = 800; //maximum number of icons to buffer


#ifdef FFS_WIN
Zstring getFileExtension(const Zstring& filename)
{
    const Zstring shortName = afterLast(filename, Zchar('\\')); //warning: using windows file name separator!

    return shortName.find(Zchar('.')) != Zstring::npos ?
           filename.AfterLast(Zchar('.')) :
           Zstring();
}


namespace
{
std::set<Zstring, LessFilename> exceptions; //thread-safe!
boost::once_flag once = BOOST_ONCE_INIT;    //
}

//test for extension for icons that physically have to be retrieved from disc
bool isPriceyExtension(const Zstring& extension)
{
    boost::call_once(once, []()
    {
        exceptions.insert(L"exe");
        exceptions.insert(L"lnk");
        exceptions.insert(L"ico");
        exceptions.insert(L"ani");
        exceptions.insert(L"cur");
        exceptions.insert(L"url");
        exceptions.insert(L"msc");
        exceptions.insert(L"scr");
    });

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
                                                      ::gdk_pixbuf_copy(other.handle_) //create new Pix buf with reference count 1 or return 0 on error
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
            ::g_object_unref(handle_);
#endif
    }

    void swap(IconHolder& other) { std::swap(handle_, other.handle_); } //throw()

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


IconHolder getAssociatedIcon(const Zstring& filename)
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
IconHolder getAssociatedIconByExt(const Zstring& extension)
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


wxIcon getDirectoryIcon()
{
#ifdef FFS_WIN
    wxIcon folderIcon;

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

    return folderIcon;

#elif defined FFS_LINUX
    return ::getAssociatedIcon(Zstr("/usr/")).toWxIcon(); //all directories will look like "/usr/"
#endif
}


wxIcon getFileIcon()
{
    wxIcon fileIcon;

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
    return fileIcon;
}


//################################################################################################################################################
//---------------------- Shared Data -------------------------
struct WorkLoad
{
public:
    Zstring extractNextFile() //context of worker thread, blocking
    {
        boost::unique_lock<boost::mutex> dummy(lockFiles);

        while (filesToLoad.empty())
            conditionNewFiles.timed_wait(dummy, boost::get_system_time() + boost::posix_time::milliseconds(50)); //interruption point!

        Zstring fileName = filesToLoad.back();
        filesToLoad.pop_back();
        return fileName;
    }

    void setWorkload(const std::vector<Zstring>& newLoad) //context of main thread
    {
        boost::unique_lock<boost::mutex> dummy(lockFiles);
        filesToLoad = newLoad;

        conditionNewFiles.notify_one();
        //condition handling, see: http://www.boost.org/doc/libs/1_43_0/doc/html/thread/synchronization.html#thread.synchronization.condvar_ref
    }

private:
    std::vector<Zstring>      filesToLoad; //processes last elements of vector first!
    boost::mutex              lockFiles;
    boost::condition_variable conditionNewFiles; //signal event: data for processing available
};


typedef std::map<Zstring, IconHolder, LessFilename> NameIconMap; //entryName/icon -> note: Zstring is thread-safe
typedef std::queue<Zstring> IconDbSequence; //entryName

class Buffer
{
public:
    bool requestFileIcon(const Zstring& fileName, wxIcon* icon = NULL);
    void insertIntoBuffer(const Zstring& entryName, const IconHolder& icon); //called by worker thread

private:
    boost::mutex   lockBuffer;
    NameIconMap    iconMappping; //use synchronisation when accessing this!
    IconDbSequence iconSequence; //save sequence of buffer entry to delete oldest elements
};
//------------------------------------------------------------


bool Buffer::requestFileIcon(const Zstring& fileName, wxIcon* icon) //context of main AND worker thread
{
    boost::lock_guard<boost::mutex> dummy(lockBuffer);

#ifdef FFS_WIN
    //"pricey" extensions are stored with fullnames and are read from disk, while cheap ones require just the extension
    const Zstring extension    = getFileExtension(fileName);
    const Zstring searchString = isPriceyExtension(extension) ? fileName : extension;
    auto iter = iconMappping.find(searchString);
#elif defined FFS_LINUX
    auto iter = iconMappping.find(fileName);
#endif

    if (iter == iconMappping.end())
        return false;

    if (icon != NULL)
        *icon = iter->second.toWxIcon();
    return true;
}


void Buffer::insertIntoBuffer(const Zstring& entryName, const IconHolder& icon) //called by worker thread
{
    boost::lock_guard<boost::mutex> dummy(lockBuffer);

    //thread saftey: icon uses ref-counting! But is NOT shared with main thread!
    auto rc = iconMappping.insert(std::make_pair(entryName, icon));
    if (rc.second) //if insertion took place
        iconSequence.push(entryName); //note: sharing Zstring with IconDB!!!

    assert(iconMappping.size() == iconSequence.size());

    //remove elements if buffer becomes too big:
    if (iconMappping.size() > BUFFER_SIZE_MAX) //limit buffer size: critical because GDI resources are limited (e.g. 10000 on XP per process)
    {
        //remove oldest element
        iconMappping.erase(iconSequence.front());
        iconSequence.pop();
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
    std::shared_ptr<WorkLoad> workload_; //main/worker thread may access different shared_ptr instances safely (even though they have the same target!)
    std::shared_ptr<Buffer> buffer_;     //http://www.boost.org/doc/libs/1_43_0/libs/smart_ptr/shared_ptr.htm?sess=8153b05b34d890e02d48730db1ff7ddc#ThreadSafety
};


void WorkerThread::operator()() //thread entry
{
    //failure to initialize COM for each thread is a source of hard to reproduce bugs: https://sourceforge.net/tracker/?func=detail&aid=3160472&group_id=234430&atid=1093080
#ifdef FFS_WIN
    ::CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    Loki::ScopeGuard dummy = Loki::MakeGuard([]() { ::CoUninitialize(); });
    (void)dummy;
#endif

    while (true)
    {
        boost::this_thread::interruption_point();

        const Zstring fileName = workload_->extractNextFile(); //start work: get next icon to load

        if (buffer_->requestFileIcon(fileName))
            continue; //icon already in buffer: skip

#ifdef FFS_WIN
        const Zstring extension = getFileExtension(fileName);
        if (isPriceyExtension(extension)) //"pricey" extensions are stored with fullnames and are read from disk, while cheap ones require just the extension
            buffer_->insertIntoBuffer(fileName, getAssociatedIcon(fileName));
        else //no read-access to disk! determine icon by extension
            buffer_->insertIntoBuffer(extension, getAssociatedIconByExt(extension));

#elif defined FFS_LINUX
        const IconHolder newIcon = getAssociatedIcon(fileName);
        buffer_->insertIntoBuffer(fileName, newIcon);
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


IconBuffer& IconBuffer::getInstance()
{
    static IconBuffer instance;
    return instance;
}

bool IconBuffer::requestFileIcon(const Zstring& fileName, wxIcon* icon) { return pimpl->buffer->requestFileIcon(fileName, icon); }

void IconBuffer::setWorkload(const std::vector<Zstring>& load) { pimpl->workload->setWorkload(load); }

const wxIcon& IconBuffer::getDirectoryIcon()
{
    static wxIcon dirIcon = ::getDirectoryIcon();
    return dirIcon;
}

const wxIcon& IconBuffer::getFileIcon()
{
    static wxIcon fileIcon = ::getFileIcon();
    return fileIcon;
}
