// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef XMLPROCESSING_H_0813748158321813490
#define XMLPROCESSING_H_0813748158321813490

#include <vector>
#include <zen/xml_io.h>
#include <zen/zstring.h>

namespace xmlAccess
{
struct XmlRealConfig
{
    XmlRealConfig() : delay(10) {}
    std::vector<Zstring> directories;
    Zstring commandline;
    unsigned int delay;
};

void readConfig(const Zstring& filename, XmlRealConfig& config, std::wstring& warningMsg); //throw FileError
void writeConfig(const XmlRealConfig& config, const Zstring& filename); //throw FileError


//reuse (some of) FreeFileSync's xml files
void readRealOrBatchConfig(const Zstring& filename, xmlAccess::XmlRealConfig& config, std::wstring& warningMsg); //throw FileError

int getProgramLanguage();
}

#endif //XMLPROCESSING_H_0813748158321813490
