// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "icon_buffer.h"
#include <queue>
#include <set>
#include <zen/thread.h> //includes <boost/thread.hpp>
#include <zen/scope_guard.h>

#ifdef ZEN_WIN
#include <zen/dll.h>
#include <zen/win_ver.h>
#include <wx/image.h>
#include "Thumbnail/thumbnail.h"

#elif defined ZEN_LINUX
#include <gtk/gtk.h>

#elif defined ZEN_MAC
#include "osx_file_icon.h"
#endif

using namespace zen;


namespace
{
const size_t BUFFER_SIZE_MAX = 600; //maximum number of icons to hold in buffer

#ifndef NDEBUG
boost::thread::id mainThreadId = boost::this_thread::get_id();
#endif

#ifdef ZEN_WIN
const bool isXpOrLater = winXpOrLater(); //VS2010 compiled DLLs are not supported on Win 2000: Popup dialog "DecodePointer not found"

#define DEF_DLL_FUN(name) const auto name = isXpOrLater ? DllFun<thumb::FunType_##name>(thumb::getDllName(), thumb::funName_##name) : DllFun<thumb::FunType_##name>();
DEF_DLL_FUN(getIconByIndex);   //
DEF_DLL_FUN(getThumbnail);     //let's spare the boost::call_once hustle and allocate statically
DEF_DLL_FUN(releaseImageData); //
#endif

class IconHolder //handle HICON/GdkPixbuf ownership supporting thread-safe usage (in contrast to wxIcon/wxBitmap)
{
public:
#ifdef ZEN_WIN
    typedef const thumb::ImageData* HandleType;
#elif defined ZEN_LINUX
    typedef GdkPixbuf* HandleType;
#elif defined ZEN_MAC
    typedef osx::ImageData* HandleType;
#endif

    explicit IconHolder(HandleType handle = nullptr) : handle_(handle) {} //take ownership!

    IconHolder(IconHolder&& other) : handle_(other.release()) {}

    IconHolder& operator=(IconHolder other) //unifying assignment
    {
        other.swap(*this);
        return *this;
    }

    ~IconHolder()
    {
        if (handle_ != nullptr)
#ifdef ZEN_WIN
            releaseImageData(handle_); //should be checked already before creating IconHolder!
#elif defined ZEN_LINUX
            ::g_object_unref(handle_); //superseedes "::gdk_pixbuf_unref"!
#elif defined ZEN_MAC
            delete handle_;
#endif
    }

    HandleType release()
    {
        ZEN_ON_SCOPE_EXIT(handle_ = nullptr);
        return handle_;
    }

    void swap(IconHolder& other) { std::swap(handle_, other.handle_); } //throw()

    //destroys raw icon! Call from GUI thread only!
    wxBitmap extractWxBitmap()
    {
        ZEN_ON_SCOPE_EXIT(assert(!*this));
        assert(boost::this_thread::get_id() == mainThreadId );

        if (!handle_)
            return wxNullBitmap;

#ifdef ZEN_WIN
        ZEN_ON_SCOPE_EXIT(IconHolder().swap(*this)); //destroy after extraction

        //let wxImage reference data without taking ownership:
        wxImage fileIcon(handle_->width, handle_->height, handle_->rgb, true);
        fileIcon.SetAlpha(handle_->alpha, true);
        return wxBitmap(fileIcon);

#elif defined ZEN_LINUX
        return wxBitmap(release()); //ownership passed!

#elif defined ZEN_MAC
        ZEN_ON_SCOPE_EXIT(IconHolder().swap(*this)); //destroy after extraction

        //let wxImage reference data without taking ownership:
        if (!handle_->rgb.empty())
        {
            wxImage fileIcon(handle_->width, handle_->height, &handle_->rgb[0], true);
            if (!handle_->alpha.empty())
                fileIcon.SetAlpha(&handle_->alpha[0], true);
            return wxBitmap(fileIcon);
        }
        assert(false); //rgb and alpha should never be empty
        return wxBitmap();
#endif
    }

private:
    HandleType handle_;

    IconHolder(const IconHolder& other); //move semantics!
    struct ConversionToBool { int dummy; };
public:
    //use member pointer as implicit conversion to bool (C++ Templates - Vandevoorde/Josuttis; chapter 20)
    operator int ConversionToBool::* () const { return handle_ != nullptr ? &ConversionToBool::dummy : nullptr; }
};


#ifdef ZEN_WIN
Zstring getFileExtension(const Zstring& filename)
{
    const Zstring shortName = afterLast(filename, Zchar('\\')); //warning: using windows file name separator!

    return contains(shortName, Zchar('.')) ?
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
        priceyExtensions.insert(L"ico");
        priceyExtensions.insert(L"ani");
        priceyExtensions.insert(L"cur");
        priceyExtensions.insert(L"msc");
        priceyExtensions.insert(L"scr");

        priceyExtensions.insert(L"lnk"); //
        priceyExtensions.insert(L"url"); //make sure shortcuts are pricey to get them to be detected by SHGetFileInfo
        priceyExtensions.insert(L"pif"); //
        priceyExtensions.insert(L"website"); //

    });
    return priceyExtensions.find(extension) == priceyExtensions.end();
}

const bool wereVistaOrLater = vistaOrLater(); //thread-safety: init at startup


thumb::IconSizeType getThumbSizeType(IconBuffer::IconSize sz)
{
    using namespace thumb;
    switch (sz)
    {
        case IconBuffer::SIZE_SMALL:
            return ICON_SIZE_16;
        case IconBuffer::SIZE_MEDIUM:
            if (!wereVistaOrLater) return ICON_SIZE_32; //48x48 doesn't look sharp on XP
            return ICON_SIZE_48;
        case IconBuffer::SIZE_LARGE:
            return ICON_SIZE_128;
    }
    return ICON_SIZE_16;
}


IconHolder getIconByAttribute(LPCWSTR pszPath, DWORD dwFileAttributes, IconBuffer::IconSize sz)
{
    //NOTE: CoInitializeEx()/CoUninitialize() needs to be called for THIS thread!
    SHFILEINFO fileInfo = {}; //initialize hIcon
    DWORD_PTR imgList = ::SHGetFileInfo(pszPath, //Windows 7 doesn't like this parameter to be an empty string
                                        dwFileAttributes,
                                        &fileInfo,
                                        sizeof(fileInfo),
                                        SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);
    if (!imgList) //no need to IUnknown::Release() imgList!
        return IconHolder();

    if (getIconByIndex && releaseImageData)
        return IconHolder(getIconByIndex(fileInfo.iIcon, getThumbSizeType(sz)));

    return IconHolder();
}


IconHolder getAssociatedIconByExt(const Zstring& extension, IconBuffer::IconSize sz)
{
    //no read-access to disk! determine icon by extension
    return getIconByAttribute((L"dummy." + extension).c_str(), FILE_ATTRIBUTE_NORMAL, sz);
}

#elif defined ZEN_LINUX
IconHolder iconHolderFromGicon(GIcon* gicon, IconBuffer::IconSize sz)
{
    if (gicon)
        if (GtkIconTheme* defaultTheme = ::gtk_icon_theme_get_default()) //not owned!
            if (GtkIconInfo* iconInfo = ::gtk_icon_theme_lookup_by_gicon(defaultTheme, gicon, IconBuffer::getSize(sz), GTK_ICON_LOOKUP_USE_BUILTIN)) //this may fail if icon is not installed on system
            {
                ZEN_ON_SCOPE_EXIT(::gtk_icon_info_free(iconInfo);)
                if (GdkPixbuf* pixBuf = ::gtk_icon_info_load_icon(iconInfo, nullptr))
                    return IconHolder(pixBuf); //pass ownership
            }
    return IconHolder();
}
#endif
}

//################################################################################################################################################

IconHolder getThumbnailIcon(const Zstring& filename, int requestedSize) //return 0 on failure
{
#ifdef ZEN_WIN
    if (getThumbnail && releaseImageData)
        return IconHolder(getThumbnail(filename.c_str(), requestedSize));

#elif defined ZEN_LINUX
    gint width  = 0;
    gint height = 0;
    if (GdkPixbufFormat* fmt = ::gdk_pixbuf_get_file_info(filename.c_str(), &width, &height))
    {
        (void)fmt;
        if (width > 0 && height > 0 && requestedSize > 0)
        {
            int trgWidth  = width;
            int trgHeight = height;

            const int maxExtent = std::max(width, height); //don't stretch small images, but shrink large ones instead!
            if (requestedSize < maxExtent)
            {
                trgWidth  = width  * requestedSize / maxExtent;
                trgHeight = height * requestedSize / maxExtent;
            }
            if (GdkPixbuf* pixBuf = ::gdk_pixbuf_new_from_file_at_size(filename.c_str(), trgWidth, trgHeight, nullptr))
                return IconHolder(pixBuf); //pass ownership
        }
    }

#elif defined ZEN_MAC
    try
    {
        return IconHolder(new osx::ImageData(osx::getThumbnail(filename.c_str(), requestedSize))); //throw SysError
    }
    catch (zen::SysError&) {}
#endif
    return IconHolder();
}


IconHolder getGenericFileIcon(IconBuffer::IconSize sz)
{
    //we're called by getAssociatedIcon()! -> avoid endless recursion!
#ifdef ZEN_WIN
    return getIconByAttribute(L"dummy", FILE_ATTRIBUTE_NORMAL, sz);

#elif defined ZEN_LINUX
    const char* mimeFileIcons[] =
    {
        "application-x-zerosize", //Kubuntu: /usr/share/icons/oxygen/48x48/mimetypes
        "text-x-generic",         //http://live.gnome.org/GnomeArt/Tutorials/IconThemes
        "empty",            //Ubuntu: /usr/share/icons/Humanity/mimes/48
        GTK_STOCK_FILE,     //"gtk-file",
        "gnome-fs-regular", //
    };

    if (GtkIconTheme* defaultTheme = gtk_icon_theme_get_default()) //not owned!
        for (auto it = std::begin(mimeFileIcons); it != std::end(mimeFileIcons); ++it)
            if (GdkPixbuf* pixBuf = gtk_icon_theme_load_icon(defaultTheme, *it, IconBuffer::getSize(sz), GTK_ICON_LOOKUP_USE_BUILTIN, nullptr))
                return IconHolder(pixBuf); //pass ownership
    return IconHolder();

#elif defined ZEN_MAC
    try
    {
        return IconHolder(new osx::ImageData(osx::getDefaultFileIcon(IconBuffer::getSize(sz)))); //throw SysError
    }
    catch (zen::SysError&) {}
    return IconHolder();
#endif
}


IconHolder getGenericDirectoryIcon(IconBuffer::IconSize sz)
{
#ifdef ZEN_WIN
    return getIconByAttribute(L"dummy", //Windows 7 doesn't like this parameter to be an empty string!
                              FILE_ATTRIBUTE_DIRECTORY, sz);
#elif defined ZEN_LINUX
    if (GIcon* dirIcon = ::g_content_type_get_icon("inode/directory")) //should contain fallback to GTK_STOCK_DIRECTORY ("gtk-directory")
        return iconHolderFromGicon(dirIcon, sz);
    return IconHolder();

#elif defined ZEN_MAC
    try
    {
        return IconHolder(new osx::ImageData(osx::getDefaultFolderIcon(IconBuffer::getSize(sz)))); //throw SysError
    }
    catch (zen::SysError&) { return IconHolder(); }
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
            if (IconHolder ico = getThumbnailIcon(filename, IconBuffer::getSize(sz)))
                return ico;
            //else: fallback to non-thumbnail icon
            break;
    }

    warn_static("problem: für folder links ist getThumbnail erfolgreich => SFGAO_LINK nicht gecheckt!")

    //2. retrieve file icons
#ifdef ZEN_WIN
    //perf: optimize fallback case for SIZE_MEDIUM and SIZE_LARGE:
    const Zstring& extension = getFileExtension(filename);
    if (isCheapExtension(extension)) //"pricey" extensions are stored with fullnames and are read from disk, while cheap ones require just the extension
        return getAssociatedIconByExt(extension, sz);
    //result will not be buffered under extension name, but full filename; this is okay, since we're in SIZE_MEDIUM or SIZE_LARGE context,
    //which means the access to get thumbnail failed: thumbnail failure is not dependent from extension in general!

    SHFILEINFO fileInfo = {};
    if (DWORD_PTR imgList = ::SHGetFileInfo(filename.c_str(), //_In_     LPCTSTR pszPath, -> note: ::SHGetFileInfo() can't handle \\?\-prefix!
                                            0,                //DWORD dwFileAttributes,
                                            &fileInfo,        //_Inout_  SHFILEINFO *psfi,
                                            sizeof(fileInfo), //UINT cbFileInfo,
                                            SHGFI_SYSICONINDEX /*| SHGFI_ATTRIBUTES*/)) //UINT uFlags
    {
        (void)imgList;
        //imgList->Release(); //empiric study: crash on XP if we release this! Seems we do not own it... -> also no GDI leak on Win7 -> okay
        //another comment on http://msdn.microsoft.com/en-us/library/bb762179(v=VS.85).aspx describes exact same behavior on Win7/XP

        //Quote: "The IImageList pointer type, such as that returned in the ppv parameter, can be cast as an HIMAGELIST as needed;
        //        for example, for use in a list view. Conversely, an HIMAGELIST can be cast as a pointer to an IImageList."
        //http://msdn.microsoft.com/en-us/library/windows/desktop/bb762185(v=vs.85).aspx

#ifdef __MINGW32__ //Shobjidl.h
#define SFGAO_LINK 0x00010000L     // Shortcut (link) or symlinks
#endif

        warn_static("support SFGAO_GHOSTED or hidden?")
        //requires SHGFI_ATTRIBUTES
        //const bool isLink = (fileInfo.dwAttributes & SFGAO_LINK) != 0;

        if (getIconByIndex && releaseImageData)
            if (const thumb::ImageData* imgData = getIconByIndex(fileInfo.iIcon, getThumbSizeType(sz)))
                return IconHolder(imgData);
    }

#elif defined ZEN_LINUX
    GFile* file = ::g_file_new_for_path(filename.c_str()); //documented to "never fail"
    ZEN_ON_SCOPE_EXIT(::g_object_unref(file);)

    if (GFileInfo* fileInfo = ::g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_ICON, G_FILE_QUERY_INFO_NONE, nullptr, nullptr))
    {
        ZEN_ON_SCOPE_EXIT(::g_object_unref(fileInfo);)
        if (GIcon* gicon = ::g_file_info_get_icon(fileInfo)) //not owned!
            return iconHolderFromGicon(gicon, sz);
    }
    //need fallback: icon lookup may fail because some icons are currently not present on system

#elif defined ZEN_MAC
    try
    {
        return IconHolder(new osx::ImageData(osx::getFileIcon(filename.c_str(), IconBuffer::getSize(sz)))); //throw SysError
    }
    catch (zen::SysError&) { assert(false); }
#endif
    return ::getGenericFileIcon(sz); //make sure this does not internally call getAssociatedIcon("someDefaultFile.txt")!!! => endless recursion!
}

//################################################################################################################################################

//---------------------- Shared Data -------------------------
class WorkLoad
{
public:
    Zstring extractNextFile() //context of worker thread, blocking
    {
        assert(boost::this_thread::get_id() != mainThreadId );
        boost::unique_lock<boost::mutex> dummy(lockFiles);

        while (filesToLoad.empty())
            conditionNewFiles.timed_wait(dummy, boost::posix_time::milliseconds(50)); //interruption point!

        Zstring fileName = filesToLoad.back();
        filesToLoad.pop_back();
        return fileName;
    }

    void setWorkload(const std::list<Zstring>& newLoad) //context of main thread
    {
        assert(boost::this_thread::get_id() == mainThreadId );
        {
            boost::lock_guard<boost::mutex> dummy(lockFiles);
            filesToLoad = newLoad;
        }
        conditionNewFiles.notify_all(); //instead of notify_one(); workaround bug: https://svn.boost.org/trac/boost/ticket/7796
        //condition handling, see: http://www.boost.org/doc/libs/1_43_0/doc/html/thread/synchronization.html#thread.synchronization.condvar_ref
    }

    void addToWorkload(const Zstring& newEntry) //context of main thread
    {
        assert(boost::this_thread::get_id() == mainThreadId );
        {
            boost::lock_guard<boost::mutex> dummy(lockFiles);
            filesToLoad.push_back(newEntry); //set as next item to retrieve
        }
        conditionNewFiles.notify_all();
    }

private:
    std::list<Zstring>        filesToLoad; //processes last elements of vector first!
    boost::mutex              lockFiles;
    boost::condition_variable conditionNewFiles; //signal event: data for processing available
};


class Buffer
{
public:
    //called by main and worker thread:
    bool hasFileIcon(const Zstring& fileName) const
    {
        boost::lock_guard<boost::mutex> dummy(lockIconList);
        return iconList.find(fileName) != iconList.end();
    }

    //must be called by main thread only! => wxBitmap is NOT thread-safe like an int (non-atomic ref-count!!!)
    Opt<wxBitmap> retrieveFileIcon(const Zstring& fileName)
    {
        assert(boost::this_thread::get_id() == mainThreadId);
        boost::lock_guard<boost::mutex> dummy(lockIconList);
        auto it = iconList.find(fileName);
        if (it == iconList.end())
            return NoValue();

        IconData& idata = it->second;
        if (idata.iconRaw) //if not yet converted...
        {
            idata.iconFmt = make_unique<wxBitmap>(idata.iconRaw.extractWxBitmap()); //convert in main thread!
            assert(!idata.iconRaw);
        }
        return idata.iconFmt ? *idata.iconFmt : wxNullBitmap; //idata.iconRaw may be inserted as empty from worker thread!
    }

    //must be called by main thread only! => ~wxBitmap() is NOT thread-safe!
    //call at an appropriate time, e.g.	after Workload::setWorkload()
    void limitBufferSize() //critical because GDI resources are limited (e.g. 10000 on XP per process)
    {
        assert(boost::this_thread::get_id() == mainThreadId);
        boost::lock_guard<boost::mutex> dummy(lockIconList);
        while (iconList.size() > BUFFER_SIZE_MAX)
        {
            iconList.erase(iconSequence.front()); //remove oldest element
            iconSequence.pop();
        }
    }

    //called by main and worker thread:
    void moveIntoBuffer(const Zstring& entryName, IconHolder&& icon)
    {
        boost::lock_guard<boost::mutex> dummy(lockIconList);

        //thread safety: moving IconHolder is free from side effects, but ~wxBitmap() is NOT! => do NOT delete items from iconList here!
        auto rc = iconList.insert(std::make_pair(entryName, IconData(std::move(icon))));
        if (rc.second) //if insertion took place
            iconSequence.push(entryName); //note: sharing Zstring with IconDB!!!

        assert(iconList.size() == iconSequence.size());
    }

private:
    struct IconData
    {
        IconData(IconHolder&& tmp) : iconRaw(std::move(tmp)) {}
        IconData(IconData&&   tmp) : iconRaw(std::move(tmp.iconRaw)), iconFmt(std::move(tmp.iconFmt))  {}

        IconHolder iconRaw; //native icon representation: may be used by any thread

        std::unique_ptr<wxBitmap> iconFmt; //use ONLY from main thread!
        //wxBitmap is NOT thread-safe: non-atomic ref-count just to begin with...
        //- prohibit implicit calls to wxBitmap(const wxBitmap&)
        //- prohibit calls to ~wxBitmap() and transitively ~IconData()
        //- prohibit even wxBitmap() default constructor - better be safe than sorry!
    };

    mutable boost::mutex lockIconList;
    std::map<Zstring, IconData, LessFilename> iconList; //shared resource; Zstring is thread-safe like an int
    std::queue<Zstring> iconSequence; //save sequence of buffer entry to delete oldest elements
};

//################################################################################################################################################

class WorkerThread //lifetime is part of icon buffer
{
public:
    WorkerThread(const std::shared_ptr<WorkLoad>& workload,
                 const std::shared_ptr<Buffer>& buffer,
                 IconBuffer::IconSize st) :
        workload_(workload),
        buffer_(buffer),
        iconSizeType(st) {}

    void operator()(); //thread entry

private:
    std::shared_ptr<WorkLoad> workload_; //main/worker thread may access different shared_ptr instances safely (even though they have the same target!)
    std::shared_ptr<Buffer> buffer_;     //http://www.boost.org/doc/libs/1_43_0/libs/smart_ptr/shared_ptr.htm?sess=8153b05b34d890e02d48730db1ff7ddc#ThreadSafety
    const IconBuffer::IconSize iconSizeType;
};


void WorkerThread::operator()() //thread entry
{
    //failure to initialize COM for each thread is a source of hard to reproduce bugs: https://sourceforge.net/tracker/?func=detail&aid=3160472&group_id=234430&atid=1093080
#ifdef ZEN_WIN
    //Prerequisites, see thumbnail.h

    //1. Initialize COM
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    ZEN_ON_SCOPE_EXIT(::CoUninitialize());

    //2. Initialize system image list
    typedef BOOL (WINAPI* FileIconInitFun)(BOOL fRestoreCache);
    const SysDllFun<FileIconInitFun> fileIconInit(L"Shell32.dll", reinterpret_cast<LPCSTR>(660)); //MS requires and documents this magic number
    assert(fileIconInit);
    if (fileIconInit)
        fileIconInit(false); //TRUE to restore the system image cache from disk; FALSE otherwise.
#endif

    while (true)
    {
        boost::this_thread::interruption_point();

        const Zstring fileName = workload_->extractNextFile(); //start work: blocks until next icon to load is retrieved

        if (!buffer_->hasFileIcon(fileName)) //perf: workload may contain duplicate entries?
            buffer_->moveIntoBuffer(fileName, getAssociatedIcon(fileName, iconSizeType));
    }
}

//#########################  redirect to impl  #####################################################

struct IconBuffer::Pimpl
{
    Pimpl() :
        workload(std::make_shared<WorkLoad>()),
        buffer  (std::make_shared<Buffer>()) {}

    std::shared_ptr<WorkLoad> workload;
    std::shared_ptr<Buffer> buffer;

    boost::thread worker;
};


IconBuffer::IconBuffer(IconSize sz) : pimpl(make_unique<Pimpl>()),
    iconSizeType(sz),
    genDirIcon (::getGenericDirectoryIcon(sz).extractWxBitmap()),
    genFileIcon(::getGenericFileIcon     (sz).extractWxBitmap())
{
    pimpl->worker = boost::thread(WorkerThread(pimpl->workload, pimpl->buffer, sz));
}


IconBuffer::~IconBuffer()
{
    setWorkload(std::list<Zstring>()); //make sure interruption point is always reached!
    pimpl->worker.interrupt();
    pimpl->worker.join(); //we assume precondition "worker.joinable()"!!!
}


int IconBuffer::getSize(IconSize icoSize)
{
    switch (icoSize)
    {
        case IconBuffer::SIZE_SMALL:
#if defined ZEN_WIN || defined ZEN_MAC
            return 16;
#elif defined ZEN_LINUX
            return 24;
#endif
        case IconBuffer::SIZE_MEDIUM:
#ifdef ZEN_WIN
            if (!wereVistaOrLater) return 32; //48x48 doesn't look sharp on XP
#endif
            return 48;

        case IconBuffer::SIZE_LARGE:
            return 128;
    }
    assert(false);
    return 0;
}


bool IconBuffer::readyForRetrieval(const Zstring& filename)
{
#ifdef ZEN_WIN
    if (iconSizeType == IconBuffer::SIZE_SMALL)
        if (isCheapExtension(getFileExtension(filename)))
            return true;
#endif
    return pimpl->buffer->hasFileIcon(filename);
}


Opt<wxBitmap> IconBuffer::retrieveFileIcon(const Zstring& filename)
{
#ifdef ZEN_WIN
    //perf: let's read icons which don't need file access right away! No async delay justified!
    if (iconSizeType == IconBuffer::SIZE_SMALL) //non-thumbnail view, we need file type icons only!
    {
        const Zstring& extension = getFileExtension(filename);
        if (isCheapExtension(extension)) //"pricey" extensions are stored with fullnames and are read from disk, while cheap ones require just the extension
        {
            if (Opt<wxBitmap> ico = pimpl->buffer->retrieveFileIcon(extension))
                return ico;

            //make sure icon is in buffer, even if icon needs not be retrieved!
            pimpl->buffer->moveIntoBuffer(extension, getAssociatedIconByExt(extension, iconSizeType));

            Opt<wxBitmap> ico = pimpl->buffer->retrieveFileIcon(extension);
            assert(ico);
            return ico;
        }
    }
#endif

    if (Opt<wxBitmap> ico = pimpl->buffer->retrieveFileIcon(filename))
        return ico;

    //since this icon seems important right now, we don't want to wait until next setWorkload() to start retrieving
    pimpl->workload->addToWorkload(filename);
    pimpl->buffer->limitBufferSize();
    return NoValue();
}


void IconBuffer::setWorkload(const std::list<Zstring>& load)
{
    pimpl->workload->setWorkload(load); //since buffer can only increase due to new workload,
    pimpl->buffer->limitBufferSize();   //this is the place to impose the limit from main thread!
}
