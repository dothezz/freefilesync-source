// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
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
    ColumnAttributeRim() : type_(COL_TYPE_DIRECTORY), width_(0), visible_(false) {}
    ColumnAttributeRim(ColumnTypeRim type, int width, bool visible) : type_(type), width_(width), visible_(visible) {}

    ColumnTypeRim type_;
    int           width_; //negative value stretches proportionally!
    bool          visible_;
};


namespace
{
std::vector<ColumnAttributeRim> getDefaultColumnAttributesLeft()
{
    std::vector<ColumnAttributeRim> attr;
    attr.push_back(ColumnAttributeRim(COL_TYPE_FULL_PATH, 250, false));
    attr.push_back(ColumnAttributeRim(COL_TYPE_DIRECTORY, 200, false));
    attr.push_back(ColumnAttributeRim(COL_TYPE_REL_PATH,  200, true));
    attr.push_back(ColumnAttributeRim(COL_TYPE_FILENAME,  150, true));
    attr.push_back(ColumnAttributeRim(COL_TYPE_SIZE,       80, true));
    attr.push_back(ColumnAttributeRim(COL_TYPE_DATE,      112, false));
    attr.push_back(ColumnAttributeRim(COL_TYPE_EXTENSION,  60, false));
    return attr;
}

std::vector<ColumnAttributeRim> getDefaultColumnAttributesRight()
{
    std::vector<ColumnAttributeRim> attr;
    attr.push_back(ColumnAttributeRim(COL_TYPE_FULL_PATH, 250, false));
    attr.push_back(ColumnAttributeRim(COL_TYPE_DIRECTORY, 200, false));
    attr.push_back(ColumnAttributeRim(COL_TYPE_REL_PATH,  200, false)); //already shown on left side
    attr.push_back(ColumnAttributeRim(COL_TYPE_FILENAME,  150, true));
    attr.push_back(ColumnAttributeRim(COL_TYPE_SIZE,       80, true));
    attr.push_back(ColumnAttributeRim(COL_TYPE_DATE,      112, false));
    attr.push_back(ColumnAttributeRim(COL_TYPE_EXTENSION,  60, false));
    return attr;
}
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
    COL_TYPE_NAVI_DIRECTORY
};


struct ColumnAttributeNavi
{
    ColumnAttributeNavi() : type_(COL_TYPE_NAVI_DIRECTORY), width_(0), visible_(false) {}
    ColumnAttributeNavi(ColumnTypeNavi type, int width, bool visible) : type_(type), width_(width), visible_(visible) {}

    ColumnTypeNavi type_;
    int            width_; //negative value stretches proportionally!
    bool           visible_;
};


static const bool defaultValueShowPercentage = true;
static const ColumnTypeNavi defaultValueLastSortColumn = COL_TYPE_NAVI_DIRECTORY; //remember sort on navigation panel
static const bool defaultValueLastSortAscending = true; //

inline
std::vector<ColumnAttributeNavi> getDefaultColumnAttributesNavi()
{
    std::vector<ColumnAttributeNavi> attr;

    ColumnAttributeNavi newEntry;

    newEntry.type_    = COL_TYPE_NAVI_DIRECTORY;
    newEntry.visible_ = true;
    newEntry.width_   = -1; //stretch, old value: 280;
    attr.push_back(newEntry);

    newEntry.type_    = COL_TYPE_NAVI_BYTES;
    newEntry.visible_ = true;
    newEntry.width_   = 60; //GTK needs a few pixels more
    attr.push_back(newEntry);

    return attr;
}
}

#endif // COL_ATTR_HEADER_189467891346732143214
