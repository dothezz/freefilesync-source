// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef FFS_FILTER_H_INCLUDED
#define FFS_FILTER_H_INCLUDED

#include "../shared/zstring.h"
#include <set>
#include <boost/shared_ptr.hpp>
#include <wx/stream.h>

namespace ffs3
{
//------------------------------------------------------------------
/*    class hierarchy:

          BaseFilter (interface)
               /|\
       _________|_____________
      |         |             |
NullFilter  NameFilter  CombinedFilter
*/

/*
Semantics of BaseFilter:
1. using it creates a NEW folder hierarchy! -> must be respected by <Automatic>-mode!
2. it applies equally to both sides => it always matches either both sides or none! => can be used while traversing a single folder!
*/

class BaseFilter //interface for filtering
{
public:
    virtual ~BaseFilter() {}

    //filtering
    virtual bool passFileFilter(const Zstring& relFilename) const = 0;
    virtual bool passDirFilter(const Zstring& relDirname, bool* subObjMightMatch) const = 0;
    //subObjMightMatch: file/dir in subdirectories could(!) match
    //note: variable is only set if passDirFilter returns false!

    virtual bool isNull() const = 0; //filter is equivalent to NullFilter, but may be technically slower

    //comparison
    bool operator<(const BaseFilter& other)  const;
    bool operator==(const BaseFilter& other) const;
    bool operator!=(const BaseFilter& other) const;

    typedef boost::shared_ptr<const BaseFilter> FilterRef; //always bound by design!

    //serialization
    void saveFilter(wxOutputStream& stream) const; //serialize derived object
    static FilterRef loadFilter(wxInputStream& stream); //CAVEAT!!! adapt this method for each new derivation!!!

private:
    virtual Zstring uniqueClassIdentifier() const = 0;   //get identifier, used for serialization
    virtual void save(wxOutputStream& stream) const = 0; //serialization
    virtual bool cmpLessSameType(const BaseFilter& other) const = 0; //typeid(*this) == typeid(other) in this context!
};


class NullFilter : public BaseFilter  //no filtering at all
{
public:
    static FilterRef load(wxInputStream& stream); //"serial constructor"
    virtual bool passFileFilter(const Zstring& relFilename) const;
    virtual bool passDirFilter(const Zstring& relDirname, bool* subObjMightMatch) const;
    virtual bool isNull() const;

private:
    virtual Zstring uniqueClassIdentifier() const;
    virtual void save(wxOutputStream& stream) const {}
    virtual bool cmpLessSameType(const BaseFilter& other) const;
};


class NameFilter : public BaseFilter  //standard filter by filename
{
public:
    NameFilter(const Zstring& includeFilter, const Zstring& excludeFilter);
    static FilterRef load(wxInputStream& stream); //"serial constructor"

    virtual bool passFileFilter(const Zstring& relFilename) const;
    virtual bool passDirFilter(const Zstring& relDirname, bool* subObjMightMatch) const;
    virtual bool isNull() const;

private:
    virtual Zstring uniqueClassIdentifier() const;
    virtual void save(wxOutputStream& stream) const;
    virtual bool cmpLessSameType(const BaseFilter& other) const;

    std::set<Zstring> filterFileIn;   //upper case (windows)
    std::set<Zstring> filterFolderIn; //
    std::set<Zstring> filterFileEx;   //
    std::set<Zstring> filterFolderEx; //

    const Zstring includeFilterTmp; //save constructor arguments for serialization
    const Zstring excludeFilterTmp; //
};


class CombinedFilter : public BaseFilter  //combine two filters to match if and only if both match
{
public:
    CombinedFilter(const FilterRef& first, const FilterRef& second) : first_(first), second_(second) {}
    static FilterRef load(wxInputStream& stream); //"serial constructor"

    virtual bool passFileFilter(const Zstring& relFilename) const;
    virtual bool passDirFilter(const Zstring& relDirname, bool* subObjMightMatch) const;
    virtual bool isNull() const;

private:
    virtual Zstring uniqueClassIdentifier() const;
    virtual void save(wxOutputStream& stream) const;
    virtual bool cmpLessSameType(const BaseFilter& other) const;

    const FilterRef first_;
    const FilterRef second_;
};


//small helper method: remove Null-filters
BaseFilter::FilterRef combineFilters(const BaseFilter::FilterRef& first,
                                     const BaseFilter::FilterRef& second);


















//---------------Inline Implementation---------------------------------------------------
inline
BaseFilter::FilterRef NullFilter::load(wxInputStream& stream) //"serial constructor"
{
    return FilterRef(new NullFilter);
}


inline
bool NullFilter::passFileFilter(const Zstring& relFilename) const
{
    return true;
}


inline
bool NullFilter::passDirFilter(const Zstring& relDirname, bool* subObjMightMatch) const
{
    assert(subObjMightMatch == NULL || *subObjMightMatch == true); //check correct usage
    return true;
}


inline
bool NullFilter::isNull() const
{
    return true;
}


inline
bool NullFilter::cmpLessSameType(const BaseFilter& other) const
{
    assert(typeid(*this) == typeid(other)); //always given in this context!
    return false;
}


inline
Zstring NullFilter::uniqueClassIdentifier() const
{
    return Zstr("NullFilter");
}


inline
bool CombinedFilter::passFileFilter(const Zstring& relFilename) const
{
    return first_->passFileFilter(relFilename) && //short-circuit behavior
           second_->passFileFilter(relFilename);
}


inline
bool CombinedFilter::passDirFilter(const Zstring& relDirname, bool* subObjMightMatch) const
{
    return first_->passDirFilter(relDirname, subObjMightMatch) && //short-circuit behavior: subObjMightMatch handled correctly!
           second_->passDirFilter(relDirname, subObjMightMatch);
}


inline
bool CombinedFilter::isNull() const
{
    return first_->isNull() && second_->isNull();
}


inline
bool CombinedFilter::cmpLessSameType(const BaseFilter& other) const
{
    assert(typeid(*this) == typeid(other)); //always given in this context!

    const CombinedFilter& otherCombFilt = static_cast<const CombinedFilter&>(other);

    if (*first_ != *otherCombFilt.first_)
        return *first_ < *otherCombFilt.first_;

    return *second_ < *otherCombFilt.second_;
}


inline
Zstring CombinedFilter::uniqueClassIdentifier() const
{
    return Zstr("CombinedFilter");
}


inline
void CombinedFilter::save(wxOutputStream& stream) const
{
    first_->saveFilter(stream);
    second_->saveFilter(stream);
}


inline
BaseFilter::FilterRef CombinedFilter::load(wxInputStream& stream) //"constructor"
{
    FilterRef first  = loadFilter(stream);
    FilterRef second = loadFilter(stream);

    return combineFilters(first, second);
}


inline
BaseFilter::FilterRef combineFilters(const BaseFilter::FilterRef& first,
                                     const BaseFilter::FilterRef& second)
{
    if (first->isNull())
    {
        if (second->isNull())
            return BaseFilter::FilterRef(new NullFilter);
        else
            return second;
    }
    else
    {
        if (second->isNull())
            return first;
        else
            return BaseFilter::FilterRef(new CombinedFilter(first, second));
    }
}


}


#endif // FFS_FILTER_H_INCLUDED

