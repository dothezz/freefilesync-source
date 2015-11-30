// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef COLUMN_ATTR_H_189467891346732143213
#define COLUMN_ATTR_H_189467891346732143213

#include <vector>

namespace zen
{
enum class ColumnTypeRim
{
    FULL_PATH,
    BASE_DIRECTORY,
    REL_FOLDER,
    FILENAME,
    SIZE,
    DATE,
    EXTENSION
};

struct ColumnAttributeRim
{
    ColumnAttributeRim() {}
    ColumnAttributeRim(ColumnTypeRim type, int offset, int stretch, bool visible) : type_(type), offset_(offset), stretch_(stretch), visible_(visible) {}

    ColumnTypeRim type_    = ColumnTypeRim::FULL_PATH;
    int           offset_  = 0;
    int           stretch_ = 0;;
    bool          visible_ = false;
};

inline
std::vector<ColumnAttributeRim> getDefaultColumnAttributesLeft()
{
    std::vector<ColumnAttributeRim> attr;
    attr.emplace_back(ColumnTypeRim::FULL_PATH,      250, 0, false);
    attr.emplace_back(ColumnTypeRim::BASE_DIRECTORY, 200, 0, false);
    attr.emplace_back(ColumnTypeRim::REL_FOLDER,    -280, 1, true); //stretch to full width and substract sum of fixed size widths!
    attr.emplace_back(ColumnTypeRim::FILENAME,       200, 0, true);
    attr.emplace_back(ColumnTypeRim::DATE,           112, 0, false);
    attr.emplace_back(ColumnTypeRim::SIZE,            80, 0, true);
    attr.emplace_back(ColumnTypeRim::EXTENSION,       60, 0, false);
    return attr;
}

inline
std::vector<ColumnAttributeRim> getDefaultColumnAttributesRight()
{
    std::vector<ColumnAttributeRim> attr;
    attr.emplace_back(ColumnTypeRim::FULL_PATH,      250, 0, false);
    attr.emplace_back(ColumnTypeRim::BASE_DIRECTORY, 200, 0, false);
    attr.emplace_back(ColumnTypeRim::REL_FOLDER  ,  -280, 1, false); //already shown on left side
    attr.emplace_back(ColumnTypeRim::FILENAME,       200, 0, true);
    attr.emplace_back(ColumnTypeRim::DATE,           112, 0, false);
    attr.emplace_back(ColumnTypeRim::SIZE,            80, 0, true);
    attr.emplace_back(ColumnTypeRim::EXTENSION,       60, 0, false);
    return attr;
}

//------------------------------------------------------------------

enum class ColumnTypeCenter
{
    CHECKBOX,
    CMP_CATEGORY,
    SYNC_ACTION,
};

//------------------------------------------------------------------

enum class ColumnTypeNavi
{
    BYTES,
    DIRECTORY,
    ITEM_COUNT
};


struct ColumnAttributeNavi
{
    ColumnAttributeNavi() {}
    ColumnAttributeNavi(ColumnTypeNavi type, int offset, int stretch, bool visible) : type_(type), offset_(offset), stretch_(stretch), visible_(visible) {}

    ColumnTypeNavi type_    = ColumnTypeNavi::DIRECTORY;
    int            offset_  = 0;
    int            stretch_ = 0;;
    bool           visible_ = false;
};


const bool defaultValueShowPercentage = true;
const ColumnTypeNavi defaultValueLastSortColumn = ColumnTypeNavi::BYTES; //remember sort on navigation panel
const bool defaultValueLastSortAscending = false;                      //

inline
std::vector<ColumnAttributeNavi> getDefaultColumnAttributesNavi()
{
    std::vector<ColumnAttributeNavi> attr;
    attr.emplace_back(ColumnTypeNavi::DIRECTORY, -120, 1, true); //stretch to full width and substract sum of fixed size widths
    attr.emplace_back(ColumnTypeNavi::ITEM_COUNT,  60, 0, true);
    attr.emplace_back(ColumnTypeNavi::BYTES,       60, 0, true); //GTK needs a few pixels width more
    return attr;
}
}

#endif //COLUMN_ATTR_H_189467891346732143213
