// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "concrete.h"
#include "native.h"
#ifdef _MSC_VER
    #include "mtp.h"
#endif

using namespace zen;


std::unique_ptr<AbstractBaseFolder> zen::createAbstractBaseFolder(const Zstring& folderPathPhrase) //noexcept
{
    //greedy: try native evaluation first
    if (acceptsFolderPathPhraseNative(folderPathPhrase)) //noexcept
        return createBaseFolderNative(folderPathPhrase); //noexcept

    //then the rest:
#ifdef _MSC_VER
    if (acceptsFolderPathPhraseMtp(folderPathPhrase)) //noexcept
        return createBaseFolderMtp(folderPathPhrase); //noexcept
#endif

    //no idea? => native!
    return createBaseFolderNative(folderPathPhrase);
}


Zstring zen::getResolvedDisplayPath(const Zstring& folderPathPhrase) //noexcept
{
    //greedy: try native evaluation first
    if (acceptsFolderPathPhraseNative(folderPathPhrase)) //noexcept
        return getResolvedDisplayPathNative(folderPathPhrase); //noexcept

    //then the rest:
#ifdef _MSC_VER
    if (acceptsFolderPathPhraseMtp(folderPathPhrase)) //noexcept
        return getResolvedDisplayPathMtp(folderPathPhrase); //noexcept
#endif

    //no idea? => native!
    return getResolvedDisplayPathNative(folderPathPhrase);
}
