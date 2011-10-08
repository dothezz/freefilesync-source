// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#ifndef PARSE_TXT_H_INCLUDED
#define PARSE_TXT_H_INCLUDED

#include "file_io.h"
#include <vector>
#include <string>

namespace zen
{
class ExtractLines
{
public:
    ExtractLines(const Zstring& filename, const std::string& lineBreak = std::string()); //throw FileError
    bool getLine(std::string& output); //throw FileError

private:
    zen::FileInput inputStream;
    std::vector<char> buffer;
    std::vector<char>::iterator bufferLogBegin;
    std::string lineBreak_;
};

}


#endif // PARSE_TXT_H_INCLUDED
