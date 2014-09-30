// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef SHADOW_H_INCLUDED
#define SHADOW_H_INCLUDED

#include <map>
#include <memory>
#include <functional>
#include <zen/zstring.h>
#include <zen/file_error.h>

namespace shadow
{
class ShadowCopy //take and buffer Windows Volume Shadow Copy snapshots as needed
{
public:
    ShadowCopy() {}

    //return filepath on shadow copy volume - follows symlinks!
    Zstring makeShadowCopy(const Zstring& inputFile, const std::function<void(const Zstring& volumeNamePf)>& onBeforeMakeVolumeCopy); //throw FileError

private:
    ShadowCopy           (const ShadowCopy&) = delete;
    ShadowCopy& operator=(const ShadowCopy&) = delete;

    class ShadowVolume;
    typedef std::map<Zstring, std::shared_ptr<ShadowVolume>, LessFilename> VolNameShadowMap;
    VolNameShadowMap shadowVol;
};
}

#endif //SHADOW_H_INCLUDED
