// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef FS_NATIVE_183247018532434563465
#define FS_NATIVE_183247018532434563465

#include "abstract.h"

namespace zen
{
bool acceptsFolderPathPhraseNative(const Zstring& folderPathPhrase); //noexcept
Zstring getResolvedDisplayPathNative(const Zstring& folderPathPhrase); //noexcept

std::unique_ptr<AbstractBaseFolder> createBaseFolderNative(const Zstring& folderPathPhrase); //noexcept
}

#endif //FS_NATIVE_183247018532434563465
