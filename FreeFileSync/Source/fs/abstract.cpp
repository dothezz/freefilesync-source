// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "abstract.h"
//#include <zen/string_tools.h>

using namespace zen;
using ABF = AbstractBaseFolder;


ABF::FileAttribAfterCopy ABF::copyFileAsStream(const AbstractPathRef& apSource, const AbstractPathRef& apTarget, //throw FileError, ErrorTargetExisting, ErrorFileLocked
                                               const std::function<void(std::int64_t bytesDelta)>& onNotifyCopyStatus)
{
    auto streamIn = getInputStream(apSource); //throw FileError, ErrorFileLocked
    if (onNotifyCopyStatus) onNotifyCopyStatus(0); //throw X!

    std::uint64_t      bytesWritten = 0;
    std::uint64_t      fileSizeExpected = streamIn->getFileSize        (); //throw FileError
    const std::int64_t modificationTime = streamIn->getModificationTime(); //throw FileError
    const FileId       sourceFileId     = streamIn->getFileId          (); //throw FileError
    FileId             targetFileId;

    auto streamOut = getOutputStream(apTarget, &fileSizeExpected, &modificationTime); //throw FileError, ErrorTargetExisting
    if (onNotifyCopyStatus) onNotifyCopyStatus(0); //throw X!

    std::vector<char> buffer(std::min(streamIn ->optimalBlockSize(),
                                      streamOut->optimalBlockSize()));
    for (;;)
    {
        const size_t bytesRead = streamIn->read(&buffer[0], buffer.size()); //throw FileError
        assert(bytesRead <= buffer.size());

        streamOut->write(&buffer[0], bytesRead); //throw FileError
        bytesWritten += bytesRead;

        if (onNotifyCopyStatus)
            onNotifyCopyStatus(bytesRead); //throw X!

        if (bytesRead != buffer.size()) //end of file
            break;
    }

    if (bytesWritten != fileSizeExpected)
        throw FileError(replaceCpy(_("Cannot read file %x."), L"%x", fmtFileName(ABF::getDisplayPath(apSource))),
                        replaceCpy(replaceCpy(_("Unexpected size of data stream.\nExpected: %x bytes\nActual: %y bytes"), L"%x", numberTo<std::wstring>(fileSizeExpected)), L"%y", numberTo<std::wstring>(bytesWritten)));

    //modification time should be set here!
    targetFileId = streamOut->finalize([&] { if (onNotifyCopyStatus) onNotifyCopyStatus(0); /*throw X*/ }); //throw FileError

    ABF::FileAttribAfterCopy attr = {};
    attr.fileSize         = bytesWritten;
    attr.modificationTime = modificationTime;
    attr.sourceFileId     = sourceFileId;
    attr.targetFileId     = targetFileId;

    return attr;
}


const Zchar* ABF::TEMP_FILE_ENDING = Zstr(".ffs_tmp");


ABF::FileAttribAfterCopy ABF::copyFileTransactional(const AbstractPathRef& apSource, const AbstractPathRef& apTarget, //throw FileError, ErrorFileLocked
                                                    bool copyFilePermissions,
                                                    bool transactionalCopy,
                                                    const std::function<void()>& onDeleteTargetFile,
                                                    const std::function<void(std::int64_t bytesDelta)>& onNotifyCopyStatus)
{
    auto copyFileBestEffort = [&](const AbstractPathRef& apTargetTmp)
    {
        //caveat: typeid returns static type for pointers, dynamic type for references!!!
        if (typeid(*apSource.abf) == typeid(*apTarget.abf))
            return apSource.abf->copyFileForSameAbfType(apSource.itemPathImpl, apTargetTmp, copyFilePermissions, onNotifyCopyStatus); //throw FileError, ErrorTargetExisting, ErrorFileLocked

        //fall back to stream-based file copy:
        if (copyFilePermissions)
            throw FileError(replaceCpy(_("Cannot write permissions of %x."), L"%x", fmtFileName(getDisplayPath(apTargetTmp))),
                            _("Operation not supported for different base folder types."));

        return copyFileAsStream(apSource, apTargetTmp, onNotifyCopyStatus); //throw FileError, ErrorTargetExisting, ErrorFileLocked
    };

    if (transactionalCopy)
    {
        AbstractPathRef apTargetTmp(*apTarget.abf, apTarget.itemPathImpl + TEMP_FILE_ENDING);
        ABF::FileAttribAfterCopy attr = {};

        for (int i = 0;; ++i)
            try
            {
                attr = copyFileBestEffort(apTargetTmp); //throw FileError, ErrorTargetExisting, ErrorFileLocked
                break;
            }
            catch (const ErrorTargetExisting&) //optimistic strategy: assume everything goes well, but recover on error -> minimize file accesses
            {
                if (i == 10) throw; //avoid endless recursion in pathological cases, e.g. https://sourceforge.net/p/freefilesync/discussion/open-discussion/thread/36adac33
                apTargetTmp.itemPathImpl = apTarget.itemPathImpl + Zchar('_') + numberTo<Zstring>(i) + TEMP_FILE_ENDING;
            }

        //transactional behavior: ensure cleanup; not needed before copyFileBestEffort() which is already transactional
        zen::ScopeGuard guardTempFile = zen::makeGuard([&] { try { removeFile(apTargetTmp); } catch (FileError&) {} });

        //have target file deleted (after read access on source and target has been confirmed) => allow for almost transactional overwrite
        if (onDeleteTargetFile)
            onDeleteTargetFile(); //throw X

        //perf: this call is REALLY expensive on unbuffered volumes! ~40% performance decrease on FAT USB stick!
        renameItem(apTargetTmp, apTarget); //throw FileError

        /*
        CAVEAT on FAT/FAT32: the sequence of deleting the target file and renaming "file.txt.ffs_tmp" to "file.txt" does
        NOT PRESERVE the creation time of the .ffs_tmp file, but SILENTLY "reuses" whatever creation time the old "file.txt" had!
        This "feature" is called "File System Tunneling":
        http://blogs.msdn.com/b/oldnewthing/archive/2005/07/15/439261.aspx
        http://support.microsoft.com/kb/172190/en-us
        */

        guardTempFile.dismiss();
        return attr;
    }
    else
    {
        /*
           Note: non-transactional file copy solves at least four problems:
            	-> skydrive - doesn't allow for .ffs_tmp extension and returns ERROR_INVALID_PARAMETER
            	-> network renaming issues
            	-> allow for true delete before copy to handle low disk space problems
            	-> higher performance on non-buffered drives (e.g. usb sticks)
        */
        if (onDeleteTargetFile)
            onDeleteTargetFile();

        return copyFileBestEffort(apTarget); //throw FileError, ErrorTargetExisting, ErrorFileLocked
    }
}
