// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "concrete.h"
#include "native.h"

using namespace zen;


AbstractPath zen::createAbstractPath(const Zstring& itemPathPhrase) //noexcept
{
    //greedy: try native evaluation first
    if (acceptsItemPathPhraseNative(itemPathPhrase)) //noexcept
        return createItemPathNative(itemPathPhrase); //noexcept

    //then the rest:

    //no idea? => native!
    return createItemPathNative(itemPathPhrase);
}
