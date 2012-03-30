// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#include "icon_buffer.h"
#include <queue>
#include <set>
#include <zen/thread.h> //includes <boost/thread.hpp>
#include <zen/scope_guard.h>
#include <boost/thread/once.hpp>

#ifdef FFS_WIN
#include <zen/dll.h>
#include "Thumbnail/thumbnail.h"
#include <zen/win_ver.h>

#elif defined FFS_LINUX
#include <gtk/gtk.h>
#endif

using namespace zen;


namespace
{
const size_t BUFFER_SIZE_MAX = 800; //maximum number of icons to buffer


int cvrtSize(IconBuffer::IconSize sz) //get size in pixel
{
    switch (sz)
    {
        case IconBuffer::SIZE_SMALL:
#ifdef FFS_WIN
            return 16;
#elif defined FFS_LINUX
            return 24;
#endif
        case IconBuffer::SIZE_MEDIUM:
            return 48;
        case IconBuffer::SIZE_LARGE:
            return 128;
    }
    assert(false);
    return 0;
}


class IconHolder //handle HICON/GdkPixbuf ownership WITHOUT ref-counting to allow thread-safe usage (in contrast to wxIcon)
{
public:
#ifdef FFS_WIN
    typedef HICON HandleType;
#elif defined FFS_LINUX
    typedef GdkPixbuf* HandleType;
#endif

    explicit IconHolder(HandleType handle = 0) : handle_(handle) {} //take ownership!

    //icon holder has value semantics!
    IconHolder(const IconHolder& other) : handle_(other.handle_ == nullptr ? nullptr :
#ifdef FFS_WIN
                                                      ::CopyIcon(other.handle_)
#elif defined FFS_LINUX
                                                      ::gdk_pixbuf_copy(other.handle_) //create new Pix buf with reference count 1 or return 0 on error
#endif
                                                     ) {}

    IconHolder(IconHolder&& other) : handle_(other.handle_) { other.handle_ = nullptr; }

    IconHolder& operator=(IconHolder other) //unifying assignment: no need for r-value reference optimization!
    {
        other.swap(*this);
        return *this;
    }

    ~IconHolder()
    {
        if (handle_ != nullptr)
#ifdef FFS_WIN
            ::DestroyIcon(handle_);
#elif defined FFS_LINUX
            ::g_object_unref(handle_); //superseedes "::gdk_pixbuf_unref"!
#endif
    }

    void swap(IconHolder& other) { std::swap(handle_, other.handle_); } //throw()

    wxIcon toWxIcon(int expectedSize) const //copy HandleType, caller needs to take ownership!
    {
        if (handle_ == nullptr)
            return wxNullIcon;

        IconHolder clone(*this);

        wxIcon newIcon; //attention: wxIcon uses reference counting!
#ifdef FFS_WIN
        newIcon.SetHICON(clone.handle_);

        {
            //this block costs ~0.04 ms
            ICONINFO icoInfo = {};
            if (::GetIconInfo(clone.handle_, &icoInfo))
            {
                ::DeleteObject(icoInfo.hbmMask); //nice potential for a GDI leak!
                ZEN_ON_SCOPE_EXIT(::DeleteObject(icoInfo.hbmColor)); //

                BITMAP bmpInfo = {};
                if (::GetObject(icoInfo.hbmColor, //__in   HGDIOBJ hgdiobj,
                                sizeof(BITMAP),   //__in   int cbBuffer,
                                &bmpInfo) != 0)   // __out  LPVOID lpvObject
                {
                    const int maxExtent = std::max(bmpInfo.bmWidth, bmpInfo.bmHeight);
                    if (0 < expectedSize && expectedSize < maxExtent)
                    {
                        bmpInfo.bmWidth  = bmpInfo.bmWidth  * expectedSize / maxExtent; //scale those Vista jumbo 256x256 icons down!
                        bmpInfo.bmHeight = bmpInfo.bmHeight * expectedSize / maxExtent; //
                    }
                    newIcon.SetSize(bmpInfo.bmWidth, bmpInfo.bmHeight); //wxIcon is stretched to this size
                }
            }
        }
        //no stretching for now
        //newIcon.SetSize(defaultSize, defaultSize); //icon is stretched to this size if referenced HICON differs

#elif defined FFS_LINUX                   //
        newIcon.SetPixbuf(clone.handle_); // transfer ownership!!
#endif                                    //
        clone.handle_ = nullptr;             //
        return newIcon;
    }

private:
    HandleType handle_;
    struct ConversionToBool { int dummy; };

public:
    //use member pointer as implicit conversion to bool (C++ Templates - Vandevoorde/Josuttis; chapter 20)
    operator int ConversionToBool::* () const { return handle_ != nullptr ? &ConversionToBool::dummy : nullptr; }
};


#ifdef FFS_WIN
Zstring getFileExtension(const Zstring& filename)
{
    const Zstring shortName = afterLast(filename, Zchar('\\')); //warning: using windows file name separator!

    return shortName.find(Zchar('.')) != Zstring::npos ?
           afterLast(filename, Zchar('.')) :
           Zstring();
}

std::set<Zstring, LessFilename> priceyExtensions;      //thread-safe!
boost::once_flag initExtensionsOnce = BOOST_ONCE_INIT; //


//test for extension for non-thumbnail icons that physically have to be retrieved from disc
bool isCheapExtension(const Zstring& extension)
{
    boost::call_once(initExtensionsOnce, []()
    {
        priceyExtensions.insert(L"exe");
        priceyExtensions.insert(L"lnk");
        priceyExtensions.insert(L"ico");
        priceyExtensions.insert(L"ani");
        priceyExtensions.insert(L"cur");
        priceyExtensions.insert(L"url");
        priceyExtensions.insert(L"msc");
        priceyExtensions.insert(L"scr");
    });
    return priceyExtensions.find(extension) == priceyExtensions.end();
}


const bool wereVistaOrLater = vistaOrLater(); //thread-safety: init at startup


int getShilIconType(IconBuffer::IconSize sz)
{
    switch (sz)
    {
        case IconBuffer::SIZE_SMALL:
            return SHIL_SMALL; //16x16, but the size can be customized by the user.
        case IconBuffer::SIZE_MEDIUM:
            return SHIL_EXTRALARGE; //typically 48x48, but the size can be customized by the user.
        case IconBuffer::SIZE_LARGE:
            return wereVistaOrLater ? SHIL_JUMBO //normally 256x256 pixels -> will be scaled down by IconHolder
                   : SHIL_EXTRALARGE; //XP doesn't have jumbo icons
    }
    return SHIL_SMALL;
}


DllFun<thumb::GetIconByIndexFct> getIconByIndex;
boost::once_flag initGetIconByIndexOnce = BOOST_ONCE_INIT;


IconHolder getIconByAttribute(LPCWSTR pszPath, DWORD dwFileAttributes, IconBuffer::IconSize sz)
{
    //NOTE: CoInitializeEx()/CoUninitialize() needs to be called for THIS thread!
    SHFILEINFO fileInfo = {}; //initialize hIcon
    DWORD_PTR imgList = ::SHGetFileInfo(pszPath, //Windows 7 doesn't like this parameter to be an empty string
                                        dwFileAttributes,
                                        &fileInfo,
                                        sizeof(fileInfo),
                                        SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);
    //no need to IUnknown::Release() imgList!
    if (!imgList)
        return IconHolder();

    boost::call_once(initGetIconByIndexOnce, []() //thread-safe init
    {
        getIconByIndex = DllFun<thumb::GetIconByIndexFct>(thumb::getDllName(), thumb::getIconByIndexFctName);
    });
    return IconHolder(getIconByIndex ? static_cast<HICON>(getIconByIndex(fileInfo.iIcon, getShilIconType(sz))) : nullptr);
}


IconHolder getAssociatedIconByExt(const Zstring& extension, IconBuffer::IconSize sz)
{
    //no read-access to disk! determine icon by extension
    return getIconByAttribute((Zstr("dummy.") + extension).c_str(), FILE_ATTRIBUTE_NORMAL, sz);
}


DllFun<thumb::GetThumbnailFct> getThumbnailIcon;
boost::once_flag initThumbnailOnce = BOOST_ONCE_INIT;
#endif
}
//################################################################################################################################################


IconHolder getThumbnail(const Zstring& filename, int requestedSize) //return 0 on failure
{
#ifdef FFS_WIN
    using namespace thumb;

    boost::call_once(initThumbnailOnce, []() //note: "getThumbnail" function itself is already thread-safe
    {
        getThumbnailIcon = DllFun<GetThumbnailFct>(getDllName(), getThumbnailFctName);
    });
    return IconHolder(getThumbnailIcon ? static_cast< ::HICON>(getThumbnailIcon(filename.c_str(), requestedSize)) : nullptr);

#elif defined FFS_LINUX
    GdkPixbuf* pixBuf = gdk_pixbuf_new_from_file_at_size(filename.c_str(), requestedSize, requestedSize, nullptr);
    return IconHolder(pixBuf); //pass ownership (may be 0)
#endif
}


const char* mimeFileIcons[] =
{
    "application-x-zerosize", //Kubuntu: /usr/share/icons/oxygen/48x48/mimetypes
    "empty",            //
    "gtk-file",         //Ubuntu: /usr/share/icons/Humanity/mimes/48
    "gnome-fs-regular", //
};


IconHolder getGenericFileIcon(IconBuffer::IconSize sz)
{
#ifdef FFS_WIN
    return getIconByAttribute(L"dummy", FILE_ATTRIBUTE_NORMAL, sz);

#elif defined FFS_LINUX
    const int requestedSize = cvrtSize(sz);

    if (GtkIconTheme* defaultTheme = gtk_icon_theme_get_default()) //not owned!
        for (auto iter = std::begin(mimeFileIcons); iter != std::end(mimeFileIcons); ++iter)
            if (GdkPixbuf* pixBuf = gtk_icon_theme_load_icon(defaultTheme, *iter, requestedSize, GTK_ICON_LOOKUP_USE_BUILTIN, nullptr))
                return IconHolder(pixBuf); //pass ownership (may be nullptr)
    return IconHolder();
#endif
}


IconHolder getAssociatedIcon(const Zstring& filename, IconBuffer::IconSize sz)
{
    //1. try to load thumbnails
    switch (sz)
    {
        case IconBuffer::SIZE_SMALL:
            break;
        case IconBuffer::SIZE_MEDIUM:
        case IconBuffer::SIZE_LARGE:
        {
            IconHolder ico = getThumbnail(filename, cvrtSize(sz));
            if (ico)
                return ico;
            //else: fallback to non-thumbnail icon
        }
        break;
    }

    //2. retrieve file icons
#ifdef FFS_WIN
    //perf: optimize fallback case for SIZE_MEDIUM and SIZE_LARGE:
    const Zstring& extension = getFileExtension(filename);
    if (isCheapExtension(extension)) //"pricey" extensions are stored with fullnames and are read from disk, while cheap ones require just the extension
        return getAssociatedIconByExt(extension, sz);
    //result will not be buffered under extension name, but full filename; this is okay, since we're in SIZE_MEDIUM or SIZE_LARGE context,
    //which means the access to get thumbnail failed: thumbnail failure is not dependent from extension in general!

    SHFILEINFO fileInfo = {};
    DWORD_PTR imgList = ::SHGetFileInfo(filename.c_str(), //zen::removeLongPathPrefix(fileName), //::SHGetFileInfo() can't handle \\?\-prefix!
                                        0,
                                        &fileInfo,
                                        sizeof(fileInfo),
                                        SHGFI_SYSICONINDEX);
    //Quote: "The IImageList pointer type, such as that returned in the ppv parameter, can be cast as an HIMAGELIST as
    //        needed; for example, for use in a list view. Conversely, an HIMAGELIST can be cast as a pointer to an IImageList."
    //http://msdn.microsoft.com/en-us/library/windows/desktop/bb762185(v=vs.85).aspx

    if (!imgList)
        return IconHolder();
    //imgList->Release(); //empiric study: crash on XP if we release this! Seems we do not own it... -> also no GDI leak on Win7 -> okay
    //another comment on http://msdn.microsoft.com/en-us/library/bb762179(v=VS.85).aspx describes exact same behavior on Win7/XP

    boost::call_once(initGetIconByIndexOnce, [] //thread-safe init
    {
        getIconByIndex = DllFun<thumb::GetIconByIndexFct>(thumb::getDllName(), thumb::getIconByIndexFctName);
    });
    return IconHolder(getIconByIndex ? static_cast<HICON>(getIconByIndex(fileInfo.iIcon, getShilIconType(sz))) : nullptr);

#elif defined FFS_LINUX
    const int requestedSize = cvrtSize(sz);

    GFile* file = g_file_new_for_path(filename.c_str()); //never fails
    ZEN_ON_SCOPE_EXIT(g_object_unref(file);)

    if (GFileInfo* fileInfo = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_ICON, G_FILE_QUERY_INFO_NONE, nullptr, nullptr))
    {
        ZEN_ON_SCOPE_EXIT(g_object_unref(fileInfo);)

        if (GIcon* gicon = g_file_info_get_icon(fileInfo)) //not owned!
            if (GtkIconTheme* defaultTheme = gtk_icon_theme_get_default()) //not owned!
                if (GtkIconInfo* iconInfo = gtk_icon_theme_lookup_by_gicon(defaultTheme, gicon, requestedSize, GTK_ICON_LOOKUP_USE_BUILTIN)) //this may fail if icon is not installed on system
                {
                    ZEN_ON_SCOPE_EXIT(gtk_icon_info_free(iconInfo);)
                    if (GdkPixbuf* pixBuf = gtk_icon_info_load_icon(iconInfo, nullptr))
                        return IconHolder(pixBuf); //pass ownership (may be nullptr)
                }
    }
    //fallback: icon lookup may fail because some icons are currently not present on system
    return ::getGenericFileIcon(sz);
#endif
}

/* Dependency Diagram:

getGenericFileIcon()
  /|\
   |
getAssociatedIcon()
  /|\
   |
getGenericDirectoryIcon()
*/

IconHolder getGenericDirectoryIcon(IconBuffer::IconSize sz)
{
#ifdef FFS_WIN
    return getIconByAttribute(L"dummy", //Windows 7 doesn't like this parameter to be an empty string!
                              FILE_ATTRIBUTE_DIRECTORY, sz);
#elif defined FFS_LINUX
    return ::getAssociatedIcon(Zstr("/usr/"), sz); //all directories will look like "/usr/"
#endif
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
        {
            boost::unique_lock<boost::mutex> dummy(lockFiles);
            filesToLoad = newLoad;
        }
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
    bool requestFileIcon(const Zstring& fileName, IconHolder* icon = nullptr)
    {
        boost::lock_guard<boost::mutex> dummy(lockBuffer);

        auto iter = iconMappping.find(fileName);
        if (iter != iconMappping.end())
        {
            if (icon != nullptr)
                *icon = iter->second;
            return true;
        }
        return false;
    }

    void insertIntoBuffer(const Zstring& entryName, const IconHolder& icon) //called by worker thread
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

private:
    boost::mutex   lockBuffer;
    NameIconMap    iconMappping; //use synchronisation when accessing this!
    IconDbSequence iconSequence; //save sequence of buffer entry to delete oldest elements
};
//-------------------------------------------------------------
//################################################################################################################################################

class WorkerThread //lifetime is part of icon buffer
{
public:
    WorkerThread(const std::shared_ptr<WorkLoad>& workload,
                 const std::shared_ptr<Buffer>& buffer,
                 IconBuffer::IconSize sz) :
        workload_(workload),
        buffer_(buffer),
        icoSize(sz) {}

    void operator()(); //thread entry

private:
    std::shared_ptr<WorkLoad> workload_; //main/worker thread may access different shared_ptr instances safely (even though they have the same target!)
    std::shared_ptr<Buffer> buffer_;     //http://www.boost.org/doc/libs/1_43_0/libs/smart_ptr/shared_ptr.htm?sess=8153b05b34d890e02d48730db1ff7ddc#ThreadSafety
    const IconBuffer::IconSize icoSize;
};


void WorkerThread::operator()() //thread entry
{
    //failure to initialize COM for each thread is a source of hard to reproduce bugs: https://sourceforge.net/tracker/?func=detail&aid=3160472&group_id=234430&atid=1093080
#ifdef FFS_WIN
    //Prerequisites, see thumbnail.h

    //1. Initialize COM
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    ZEN_ON_SCOPE_EXIT(::CoUninitialize());

    //2. Initialize system image list
    typedef BOOL (WINAPI* FileIconInitFun)(BOOL fRestoreCache);
    const SysDllFun<FileIconInitFun> fileIconInit(L"Shell32.dll", reinterpret_cast<LPCSTR>(660));
    assert(fileIconInit);
    if (fileIconInit)
        fileIconInit(false); //TRUE to restore the system image cache from disk; FALSE otherwise.
#endif

    while (true)
    {
        boost::this_thread::interruption_point();

        const Zstring fileName = workload_->extractNextFile(); //start work: get next icon to load

        if (buffer_->requestFileIcon(fileName))
            continue; //icon already in buffer: skip

        buffer_->insertIntoBuffer(fileName, getAssociatedIcon(fileName, icoSize));
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


IconBuffer::IconBuffer(IconSize sz) :
    pimpl(new Pimpl),
    icoSize(sz),
    genDirIcon(::getGenericDirectoryIcon(sz).toWxIcon(cvrtSize(icoSize))),
    genFileIcon(::getGenericFileIcon(sz).toWxIcon(cvrtSize(icoSize)))
{
    pimpl->worker = boost::thread(WorkerThread(pimpl->workload, pimpl->buffer, sz));
}


IconBuffer::~IconBuffer()
{
    setWorkload(std::vector<Zstring>()); //make sure interruption point is always reached!
    pimpl->worker.interrupt();
    pimpl->worker.join();
}


int IconBuffer::getSize() const
{
    return cvrtSize(icoSize);
}


bool IconBuffer::requestFileIcon(const Zstring& filename, wxIcon* icon)
{
    auto getIcon = [&](const Zstring& entryName) -> bool
    {
        if (!icon)
            return pimpl->buffer->requestFileIcon(entryName);

        IconHolder heldIcon;
        if (!pimpl->buffer->requestFileIcon(entryName, &heldIcon))
            return false;
        *icon = heldIcon.toWxIcon(cvrtSize(icoSize));
        return true;
    };

#ifdef FFS_WIN
    //perf: let's read icons which don't need file access right away! No async delay justified!
    if (icoSize == IconBuffer::SIZE_SMALL) //non-thumbnail view, we need file type icons only!
    {
        const Zstring& extension = getFileExtension(filename);
        if (isCheapExtension(extension)) //"pricey" extensions are stored with fullnames and are read from disk, while cheap ones require just the extension
        {
            if (!getIcon(extension))
            {
                IconHolder heldIcon = getAssociatedIconByExt(extension, icoSize); //fast!
                pimpl->buffer->insertIntoBuffer(extension, heldIcon);
                if (icon)
                    *icon = heldIcon.toWxIcon(cvrtSize(icoSize));
            }
            return true;
        }
    }
#endif

    return getIcon(filename);
}

void IconBuffer::setWorkload(const std::vector<Zstring>& load) { pimpl->workload->setWorkload(load); }
