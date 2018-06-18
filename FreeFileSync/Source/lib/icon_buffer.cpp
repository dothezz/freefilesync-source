// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "icon_buffer.h"
#include <set>
#include <zen/thread.h> //includes <boost/thread.hpp>
#include <zen/scope_guard.h>
#include <wx+/image_resources.h>
#include "icon_loader.h"

#ifdef ZEN_WIN
    #include <zen/win_ver.h>
#endif

using namespace zen;
using ABF = AbstractBaseFolder;


namespace
{
const size_t BUFFER_SIZE_MAX = 800; //maximum number of icons to hold in buffer: must be big enough to hold visible icons + preload buffer! Consider OS limit on GDI resources (wxBitmap)!!!

#ifndef NDEBUG
    const boost::thread::id mainThreadId = boost::this_thread::get_id();
#endif

#ifdef ZEN_WIN
    const bool wereVistaOrLater = vistaOrLater();
#endif


//destroys raw icon! Call from GUI thread only!
wxBitmap extractWxBitmap(ImageHolder&& ih)
{
    assert(boost::this_thread::get_id() == mainThreadId);
#ifndef NDEBUG
    auto check = [&] { assert(!ih); }; //work around V120_XP compilation issue
    ZEN_ON_SCOPE_EXIT(check());
#endif

    if (!ih.getRgb())
        return wxNullBitmap;

    //let wxImage take ownership:
    wxImage img(ih.getWidth(), ih.getHeight(), ih.releaseRgb(), false /*static_data*/);
    if (ih.getAlpha())
        img.SetAlpha(ih.releaseAlpha(), false);
    return wxBitmap(img);
}


#ifdef ZEN_WIN
std::set<Zstring, LessFilePath> customIconExt //function-scope statics are not (yet) thread-safe in VC12
{
    L"ani",
    L"cur",
    L"exe",
    L"ico",
    L"msc",
    L"scr"
};
std::set<Zstring, LessFilePath> linkExt
{
    L"lnk",
    L"pif",
    L"url",
    L"website"
};

//test for extension for non-thumbnail icons that can have a stock icon which does not have to be physically read from disc
inline
bool hasStandardIconExtension(const Zstring& filePath)
{
    const Zstring extension(getFileExtension(filePath));

    return customIconExt.find(extension) == customIconExt.end() &&
           linkExt.find(extension) == linkExt.end();
}
#endif
}

//################################################################################################################################################

ImageHolder getDisplayIcon(const ABF::IconLoader& iconLoader, const Zstring& templateName, IconBuffer::IconSize sz)
{
    //1. try to load thumbnails
    switch (sz)
    {
        case IconBuffer::SIZE_SMALL:
            break;
        case IconBuffer::SIZE_MEDIUM:
        case IconBuffer::SIZE_LARGE:
            if (iconLoader.getThumbnailImage)
                if (ImageHolder img = iconLoader.getThumbnailImage(IconBuffer::getSize(sz)))
                    return img;
            //else: fallback to non-thumbnail icon
            break;
    }

    //2. retrieve file icons
#ifdef ZEN_WIN
    //result will be buffered under full filepath, not extension; this is okay: failure to load thumbnail is independent from extension in general!
    if (!hasStandardIconExtension(templateName)) //"pricey" extensions are stored with full path and are read from disk, while cheap ones require just the extension
#endif
        if (iconLoader.getFileIcon)
            if (ImageHolder img = iconLoader.getFileIcon(IconBuffer::getSize(sz)))
                return img;

    //3. fallbacks
    if (ImageHolder img = getIconByTemplatePath(templateName, IconBuffer::getSize(sz)))
        return img;

    return genericFileIcon(IconBuffer::getSize(sz));
}

//################################################################################################################################################

//---------------------- Shared Data -------------------------
struct WorkItem
{
    WorkItem(const AbstractPathRef::ItemId& id, const ABF::IconLoader& iconLoader, const Zstring& fileName) : id_(id), iconLoader_(iconLoader), fileName_(fileName) {}

    AbstractPathRef::ItemId id_; //async icon loading => avoid any dangling references!!!
    ABF::IconLoader iconLoader_; //THREAD-SAFETY: thread-safe like an int!
    Zstring fileName_;           //template name for use as fallback icon
};


class WorkLoad
{
public:
    WorkItem extractNextFile() //context of worker thread, blocking
    {
        assert(boost::this_thread::get_id() != mainThreadId);
        boost::unique_lock<boost::mutex> dummy(lockFiles);

        while (workLoad.empty())
            conditionNewWork.timed_wait(dummy, boost::posix_time::milliseconds(100)); //interruption point!

        WorkItem workItem = workLoad.back(); //
        workLoad.pop_back();                 //yes, not std::bad_alloc exception-safe, but bad_alloc is not relevant for us
        return workItem;                     //
    }

    void setWorkload(const std::vector<AbstractPathRef>& newLoad) //context of main thread
    {
        assert(boost::this_thread::get_id() == mainThreadId);
        {
            boost::lock_guard<boost::mutex> dummy(lockFiles);

            workLoad.clear();
            for (const AbstractPathRef& filePath : newLoad)
                workLoad.emplace_back(filePath.getUniqueId(),
                                      ABF::getAsyncIconLoader(filePath), //noexcept!
                                      ABF::getFileShortName(filePath));  //
        }
        conditionNewWork.notify_all(); //instead of notify_one(); workaround bug: https://svn.boost.org/trac/boost/ticket/7796
        //condition handling, see: http://www.boost.org/doc/libs/1_43_0/doc/html/thread/synchronization.html#thread.synchronization.condvar_ref
    }

    void addToWorkload(const AbstractPathRef& filePath) //context of main thread
    {
        assert(boost::this_thread::get_id() == mainThreadId);
        {
            boost::lock_guard<boost::mutex> dummy(lockFiles);

            workLoad.emplace_back(filePath.getUniqueId(), //set as next item to retrieve
                                  ABF::getAsyncIconLoader(filePath), //noexcept!
                                  ABF::getFileShortName(filePath));  //
        }
        conditionNewWork.notify_all();
    }

private:
    std::vector<WorkItem>     workLoad; //processes last elements of vector first!
    boost::mutex              lockFiles;
    boost::condition_variable conditionNewWork; //signal event: data for processing available
};


class Buffer
{
public:
    Buffer() : firstInsertPos(iconList.end()), lastInsertPos(iconList.end()) {}

    //called by main and worker thread:
    bool hasIcon(const AbstractPathRef::ItemId& id) const
    {
        boost::lock_guard<boost::mutex> dummy(lockIconList);
        return iconList.find(id) != iconList.end();
    }

    //must be called by main thread only! => wxBitmap is NOT thread-safe like an int (non-atomic ref-count!!!)
    Opt<wxBitmap> retrieve(const AbstractPathRef::ItemId& id)
    {
        assert(boost::this_thread::get_id() == mainThreadId);
        boost::lock_guard<boost::mutex> dummy(lockIconList);

        auto it = iconList.find(id);
        if (it == iconList.end())
            return NoValue();

        markAsHot(it);

        IconData& idata = refData(it);
        if (idata.iconRaw) //if not yet converted...
        {
            idata.iconFmt = make_unique<wxBitmap>(extractWxBitmap(std::move(idata.iconRaw))); //convert in main thread!
            assert(!idata.iconRaw);
        }
        return idata.iconFmt ? *idata.iconFmt : wxNullBitmap; //idata.iconRaw may be inserted as empty from worker thread!
    }

    //called by main and worker thread:
    void insert(const AbstractPathRef::ItemId& id, ImageHolder&& icon)
    {
        boost::lock_guard<boost::mutex> dummy(lockIconList);

        //thread safety: moving ImageHolder is free from side effects, but ~wxBitmap() is NOT! => do NOT delete items from iconList here!
        auto rc = iconList.emplace(id, makeValueObject());
        assert(rc.second); //insertion took place
        if (rc.second)
        {
            refData(rc.first).iconRaw = std::move(icon);
            priorityListPushBack(rc.first);
        }
    }

    //must be called by main thread only! => ~wxBitmap() is NOT thread-safe!
    //call at an appropriate time, e.g.	after Workload::setWorkload()
    void limitSize()
    {
        assert(boost::this_thread::get_id() == mainThreadId);
        boost::lock_guard<boost::mutex> dummy(lockIconList);

        while (iconList.size() > BUFFER_SIZE_MAX)
        {
            auto itDelPos = firstInsertPos;
            priorityListPopFront();
            iconList.erase(itDelPos); //remove oldest element
        }
    }

private:
    struct IconData;

#ifdef __clang__ //workaround libc++ limitation for incomplete types: http://llvm.org/bugs/show_bug.cgi?id=17701
    typedef std::map<AbstractPathRef::ItemId, std::unique_ptr<IconData>> FileIconMap;
    static IconData& refData(FileIconMap::iterator it) { return *(it->second); }
    static std::unique_ptr<IconData> makeValueObject() { return make_unique<IconData>(); }
#else
    typedef std::map<AbstractPathRef::ItemId, IconData> FileIconMap;
    IconData& refData(FileIconMap::iterator it) { return it->second; }
    static IconData makeValueObject() { return IconData(); }
#endif

    //call while holding lock:
    void priorityListPopFront()
    {
        assert(firstInsertPos!= iconList.end());
        firstInsertPos = refData(firstInsertPos).next_;

        if (firstInsertPos != iconList.end())
            refData(firstInsertPos).prev_ = iconList.end();
        else //priority list size > BUFFER_SIZE_MAX in this context, but still for completeness:
            lastInsertPos = iconList.end();
    }

    //call while holding lock:
    void priorityListPushBack(FileIconMap::iterator it)
    {
        if (lastInsertPos == iconList.end())
        {
            assert(firstInsertPos == iconList.end());
            firstInsertPos = lastInsertPos = it;
            refData(it).prev_ = refData(it).next_ = iconList.end();
        }
        else
        {
            refData(it).next_ = iconList.end();
            refData(it).prev_ = lastInsertPos;
            refData(lastInsertPos).next_ = it;
            lastInsertPos = it;
        }
    }

    //call while holding lock:
    void markAsHot(FileIconMap::iterator it) //mark existing buffer entry as if newly inserted
    {
        assert(it != iconList.end());
        if (refData(it).next_ != iconList.end())
        {
            if (refData(it).prev_ != iconList.end())
            {
                refData(refData(it).prev_).next_ = refData(it).next_; //remove somewhere from the middle
                refData(refData(it).next_).prev_ = refData(it).prev_; //
            }
            else
            {
                assert(it == firstInsertPos);
                priorityListPopFront();
            }
            priorityListPushBack(it);
        }
        else
        {
            if (refData(it).prev_ != iconList.end())
                assert(it == lastInsertPos); //nothing to do
            else
                assert(iconList.size() == 1 && it == firstInsertPos && it == lastInsertPos); //nothing to do
        }
    }

    struct IconData
    {
        IconData() {}
        IconData(IconData&& tmp) : iconRaw(std::move(tmp.iconRaw)), iconFmt(std::move(tmp.iconFmt)), prev_(tmp.prev_), next_(tmp.next_) {}

        ImageHolder iconRaw; //native icon representation: may be used by any thread

        std::unique_ptr<wxBitmap> iconFmt; //use ONLY from main thread!
        //wxBitmap is NOT thread-safe: non-atomic ref-count just to begin with...
        //- prohibit implicit calls to wxBitmap(const wxBitmap&)
        //- prohibit calls to ~wxBitmap() and transitively ~IconData()
        //- prohibit even wxBitmap() default constructor - better be safe than sorry!

        FileIconMap::iterator prev_; //store list sorted by time of insertion into buffer
        FileIconMap::iterator next_; //
    };

    mutable boost::mutex lockIconList;
    FileIconMap iconList; //shared resource; Zstring is thread-safe like an int
    FileIconMap::iterator firstInsertPos;
    FileIconMap::iterator lastInsertPos;
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


class RunOnStartup
{
public:
    RunOnStartup()
    {
#ifdef ZEN_WIN
        //thumbnail.h prerequisites: 1. initialize COM, 2. initialize system image list
        typedef BOOL (WINAPI* FileIconInitFun)(BOOL fRestoreCache);
        const SysDllFun<FileIconInitFun> fileIconInit(L"Shell32.dll", reinterpret_cast<LPCSTR>(660)); //MS requires and documents this magic number
        assert(fileIconInit);
        if (fileIconInit)
            fileIconInit(true); //MSDN: "TRUE to restore the system image cache from disk; FALSE otherwise."
        /*
        	"FileIconInit's "fRestoreCache" parameter determines whether or not it loads the 48-or-so "standard" shell icons. If FALSE is specified,
        	it only loads a very minimal set of icons. [...] SHGetFileInfo internally call FileIconInit(FALSE), so if you want
        	your copy of the system image list to contain the standard icons,  you should call FileIconInit(TRUE) at startup."
        		- Jim Barry, MVP (Windows SDK)
        */
#endif
    }
} dummy;


void WorkerThread::operator()() //thread entry
{
#ifdef ZEN_WIN
    //1. Initialize COM
    if (FAILED(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) //may return S_FALSE, which is NOT failure, see MSDN
    {
        assert(false);
        return;
    }
    ZEN_ON_SCOPE_EXIT(::CoUninitialize());
#endif

    while (true)
    {
        boost::this_thread::interruption_point();

        const WorkItem workItem = workload_->extractNextFile(); //start work: blocks until next icon to load is retrieved

        if (!buffer_->hasIcon(workItem.id_)) //perf: workload may contain duplicate entries?
            buffer_->insert(workItem.id_, getDisplayIcon(workItem.iconLoader_, workItem.fileName_, iconSizeType));
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


IconBuffer::IconBuffer(IconSize sz) : pimpl(make_unique<Pimpl>()), iconSizeType(sz)
{
    pimpl->worker = boost::thread(WorkerThread(pimpl->workload, pimpl->buffer, sz));
}


IconBuffer::~IconBuffer()
{
    setWorkload({}); //make sure interruption point is always reached!
    pimpl->worker.interrupt();
    pimpl->worker.join(); //we assume precondition "worker.joinable()"!!!
}


int IconBuffer::getSize(IconSize sz)
{
    //coordinate with getThumbSizeType() and linkOverlayIcon()!
    switch (sz)
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


bool IconBuffer::readyForRetrieval(const AbstractPathRef& filePath)
{
#ifdef ZEN_WIN
    if (iconSizeType == IconBuffer::SIZE_SMALL)
        if (hasStandardIconExtension(ABF::getFileShortName(filePath)))
            return true;
#endif
    return pimpl->buffer->hasIcon(filePath.getUniqueId());
}


Opt<wxBitmap> IconBuffer::retrieveFileIcon(const AbstractPathRef& filePath)
{
#ifdef ZEN_WIN
    //perf: let's read icons which don't need file access right away! No async delay justified!
    const Zstring fileName = ABF::getFileShortName(filePath);
    if (iconSizeType == IconBuffer::SIZE_SMALL) //non-thumbnail view, we need file type icons only!
        if (hasStandardIconExtension(fileName))
            return this->getIconByExtension(fileName); //buffered!!!
#endif

    if (Opt<wxBitmap> ico = pimpl->buffer->retrieve(filePath.getUniqueId()))
        return ico;

    //since this icon seems important right now, we don't want to wait until next setWorkload() to start retrieving
    pimpl->workload->addToWorkload(filePath);
    pimpl->buffer->limitSize();
    return NoValue();
}


void IconBuffer::setWorkload(const std::vector<AbstractPathRef>& load)
{
    assert(load.size() < BUFFER_SIZE_MAX / 2);

    pimpl->workload->setWorkload(load); //since buffer can only increase due to new workload,
    pimpl->buffer->limitSize();   //this is the place to impose the limit from main thread!
}


wxBitmap IconBuffer::getIconByExtension(const Zstring& filePath)
{
    const Zstring& extension = getFileExtension(filePath);
    const AbstractPathRef::ItemId extId(nullptr, extension);

    if (Opt<wxBitmap> ico = pimpl->buffer->retrieve(extId))
        return *ico;

    const Zstring& templateName(extension.empty() ? Zstr("file") : Zstr("file.") + extension);
    //don't pass actual file name to getIconByTemplatePath(), e.g. "AUTHORS" has own mime type on Linux!!!
    pimpl->buffer->insert(extId, getIconByTemplatePath(templateName, IconBuffer::getSize(iconSizeType)));

    Opt<wxBitmap> ico = pimpl->buffer->retrieve(extId);
    assert(ico);
    return *ico;
}


wxBitmap IconBuffer::genericFileIcon(IconSize sz)
{
    return extractWxBitmap(zen::genericFileIcon(IconBuffer::getSize(sz)));
}


wxBitmap IconBuffer::genericDirIcon(IconSize sz)
{
    return extractWxBitmap(zen::genericDirIcon(IconBuffer::getSize(sz)));
}


wxBitmap IconBuffer::linkOverlayIcon(IconSize sz)
{
    //coordinate with IconBuffer::getSize()!
    return getResourceImage([sz]
    {
        const int pixelSize = IconBuffer::getSize(sz);

        if (pixelSize >= 128) return L"link_128";
        if (pixelSize >=  48) return L"link_48";
        if (pixelSize >=  32) return L"link_32";
        if (pixelSize >=  24) return L"link_24";
        return L"link_16";
    }());
}


bool zen::hasLinkExtension(const Zstring& filepath)
{
#ifdef ZEN_WIN
    const Zstring& extension = getFileExtension(filepath);
    return linkExt.find(extension) != linkExt.end();

#elif defined ZEN_LINUX
    const Zstring& extension = getFileExtension(filepath);
    return extension == "desktop";

#elif defined ZEN_MAC
    return false; //alias files already get their arrow icon via "NSWorkspace::iconForFile"
#endif
}
