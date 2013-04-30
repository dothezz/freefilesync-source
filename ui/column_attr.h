// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef COL_ATTR_HEADER_189467891346732143214
#define COL_ATTR_HEADER_189467891346732143214

#include <stddef.h> //size_t
#include <vector>

namespace zen
{
enum ColumnTypeRim
{
    COL_TYPE_DIRECTORY,
    COL_TYPE_FULL_PATH,
    COL_TYPE_REL_PATH,
    COL_TYPE_FILENAME,
    COL_TYPE_SIZE,
    COL_TYPE_DATE,
    COL_TYPE_EXTENSION
};

struct ColumnAttributeRim
{
    ColumnAttributeRim() : type_(COL_TYPE_DIRECTORY), offset_(0), stretch_(0), visible_(false) {}
    ColumnAttributeRim(ColumnTypeRim type, int offset, int stretch, bool visible) : type_(type), offset_(offset), stretch_(stretch), visible_(visible) {}

    ColumnTypeRim type_;
    int           offset_;
    int           stretch_;
    bool          visible_;
};


inline
std::vector<ColumnAttributeRim> getDefaultColumnAttributesLeft()
{
    std::vector<ColumnAttributeRim> attr;
    attr.push_back(ColumnAttributeRim(COL_TYPE_FULL_PATH,  250, 0, false));
    attr.push_back(ColumnAttributeRim(COL_TYPE_DIRECTORY,  200, 0, false));
    attr.push_back(ColumnAttributeRim(COL_TYPE_REL_PATH,   200, 0, true));
    attr.push_back(ColumnAttributeRim(COL_TYPE_FILENAME,  -280, 1, true)); //stretch to full width and substract sum of fixed size widths!
    attr.push_back(ColumnAttributeRim(COL_TYPE_DATE,       112, 0, false));
    attr.push_back(ColumnAttributeRim(COL_TYPE_SIZE,        80, 0, true));
    attr.push_back(ColumnAttributeRim(COL_TYPE_EXTENSION,   60, 0, false));
    return attr;
}

inline
std::vector<ColumnAttributeRim> getDefaultColumnAttributesRight()
{
    std::vector<ColumnAttributeRim> attr;
    attr.push_back(ColumnAttributeRim(COL_TYPE_FULL_PATH, 250, 0, false));
    attr.push_back(ColumnAttributeRim(COL_TYPE_DIRECTORY, 200, 0, false));
    attr.push_back(ColumnAttributeRim(COL_TYPE_REL_PATH,  200, 0, false)); //already shown on left side
    attr.push_back(ColumnAttributeRim(COL_TYPE_FILENAME,  -80, 1, true));  //stretch to full width and substract sum of fixed size widths!
    attr.push_back(ColumnAttributeRim(COL_TYPE_DATE,      112, 0, false));
    attr.push_back(ColumnAttributeRim(COL_TYPE_SIZE,       80, 0, true));
    attr.push_back(ColumnAttributeRim(COL_TYPE_EXTENSION,  60, 0, false));
    return attr;
}

//------------------------------------------------------------------

enum ColumnTypeMiddle
{
    COL_TYPE_MIDDLE_VALUE,
    COL_TYPE_BORDER
};

//------------------------------------------------------------------

enum ColumnTypeNavi
{
    COL_TYPE_NAVI_BYTES,
    COL_TYPE_NAVI_DIRECTORY,
    COL_TYPE_NAVI_ITEM_COUNT
};


struct ColumnAttributeNavi
{
    ColumnAttributeNavi() : type_(COL_TYPE_NAVI_DIRECTORY), offset_(0), stretch_(0), visible_(false) {}
    ColumnAttributeNavi(ColumnTypeNavi type, int offset, int stretch, bool visible) : type_(type), offset_(offset), stretch_(stretch), visible_(visible) {}

    ColumnTypeNavi type_;
    int            offset_;
    int            stretch_;
    bool           visible_;
};


const bool defaultValueShowPercentage = true;
const ColumnTypeNavi defaultValueLastSortColumn = COL_TYPE_NAVI_BYTES; //remember sort on navigation panel
const bool defaultValueLastSortAscending = false;                      //

inline
std::vector<ColumnAttributeNavi> getDefaultColumnAttributesNavi()
{
    std::vector<ColumnAttributeNavi> attr;
    attr.push_back(ColumnAttributeNavi(COL_TYPE_NAVI_DIRECTORY, -120, 1, true)); //stretch to full width and substract sum of fixed size widths
    attr.push_back(ColumnAttributeNavi(COL_TYPE_NAVI_ITEM_COUNT,  60, 0, true));
    attr.push_back(ColumnAttributeNavi(COL_TYPE_NAVI_BYTES,       60, 0, true)); //GTK needs a few pixels width more
    return attr;
}
}

#endif // COL_ATTR_HEADER_189467891346732143214
