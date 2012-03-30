// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zhnmju123 AT gmx DOT de) - All Rights Reserved    *
// **************************************************************************

#ifndef FFS_FILTER_H_INCLUDED
#define FFS_FILTER_H_INCLUDED

#include <vector>
#include <memory>
#include <wx/stream.h>
#include <zen/zstring.h>

namespace zen
{
//------------------------------------------------------------------
/*
Semantics of HardFilter:
1. using it creates a NEW folder hierarchy! -> must be considered by <Automatic>-mode! (fortunately it turns out, doing nothing already has perfect semantics :)
2. it applies equally to both sides => it always matches either both sides or none! => can be used while traversing a single folder!

    class hierarchy:

          HardFilter (interface)
               /|\
       _________|_____________
      |         |             |
NullFilter  NameFilter  CombinedFilter
*/

class HardFilter //interface for filtering
{
public:
    virtual ~HardFilter() {}

    //filtering
    virtual bool passFileFilter(const Zstring& relFilename) const = 0;
    virtual bool passDirFilter (const Zstring& relDirname, bool* subObjMightMatch) const = 0;
    //subObjMightMatch: file/dir in subdirectories could(!) match
    //note: variable is only set if passDirFilter returns false!

    virtual bool isNull() const = 0; //filter is equivalent to NullFilter, but may be technically slower

    typedef std::shared_ptr<const HardFilter> FilterRef; //always bound by design!

    //serialization
    void saveFilter(wxOutputStream& stream) const; //serialize derived object
    static FilterRef loadFilter(wxInputStream& stream); //CAVEAT!!! adapt this method for each new derivation!!!

private:
    friend bool operator< (const HardFilter& lhs, const HardFilter& rhs);

    virtual void save(wxOutputStream& stream) const = 0; //serialization
    virtual Zstring uniqueClassIdentifier() const = 0;   //get identifier, used for serialization
    virtual bool cmpLessSameType(const HardFilter& other) const = 0; //typeid(*this) == typeid(other) in this context!
};

inline bool operator==(const HardFilter& lhs, const HardFilter& rhs) { return !(lhs < rhs) && !(rhs < lhs); }
inline bool operator!=(const HardFilter& lhs, const HardFilter& rhs) { return !(lhs == rhs); }


//small helper method: merge two hard filters (thereby remove Null-filters)
HardFilter::FilterRef combineFilters(const HardFilter::FilterRef& first,
                                     const HardFilter::FilterRef& second);


class NullFilter : public HardFilter  //no filtering at all
{
public:
    virtual bool passFileFilter(const Zstring& relFilename) const;
    virtual bool passDirFilter(const Zstring& relDirname, bool* subObjMightMatch) const;
    virtual bool isNull() const;

private:
    friend class HardFilter;
    virtual void save(wxOutputStream& stream) const {}
    virtual Zstring uniqueClassIdentifier() const;
    static FilterRef load(wxInputStream& stream); //"serial constructor"
    virtual bool cmpLessSameType(const HardFilter& other) const;
};


class NameFilter : public HardFilter  //standard filter by filename
{
public:
    NameFilter(const Zstring& includeFilter, const Zstring& excludeFilter);

    virtual bool passFileFilter(const Zstring& relFilename) const;
    virtual bool passDirFilter(const Zstring& relDirname, bool* subObjMightMatch) const;

    virtual bool isNull() const;
    static bool isNull(const Zstring& includeFilter, const Zstring& excludeFilter); //*fast* check without expensively constructing NameFilter instance!

private:
    friend class HardFilter;
    virtual void save(wxOutputStream& stream) const;
    virtual Zstring uniqueClassIdentifier() const;
    static FilterRef load(wxInputStream& stream); //"serial constructor"
    virtual bool cmpLessSameType(const HardFilter& other) const;

    std::vector<Zstring> filterFileIn;   //
    std::vector<Zstring> filterFolderIn; //upper case (windows) + unique items by construction
    std::vector<Zstring> filterFileEx;   //
    std::vector<Zstring> filterFolderEx; //

    const Zstring includeFilterTmp; //save constructor arguments for serialization
    const Zstring excludeFilterTmp; //
};


class CombinedFilter : public HardFilter  //combine two filters to match if and only if both match
{
public:
    CombinedFilter(const FilterRef& first, const FilterRef& second) : first_(first), second_(second) {}

    virtual bool passFileFilter(const Zstring& relFilename) const;
    virtual bool passDirFilter(const Zstring& relDirname, bool* subObjMightMatch) const;
    virtual bool isNull() const;

private:
    friend class HardFilter;
    virtual void save(wxOutputStream& stream) const;
    virtual Zstring uniqueClassIdentifier() const;
    static FilterRef load(wxInputStream& stream); //"serial constructor"
    virtual bool cmpLessSameType(const HardFilter& other) const;

    const FilterRef first_;
    const FilterRef second_;
};


















//---------------Inline Implementation---------------------------------------------------
inline
HardFilter::FilterRef NullFilter::load(wxInputStream& stream) //"serial constructor"
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
    assert(!subObjMightMatch || *subObjMightMatch == true); //check correct usage
    return true;
}


inline
bool NullFilter::isNull() const
{
    return true;
}


inline
bool NullFilter::cmpLessSameType(const HardFilter& other) const
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
bool CombinedFilter::cmpLessSameType(const HardFilter& other) const
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
HardFilter::FilterRef CombinedFilter::load(wxInputStream& stream) //"constructor"
{
    FilterRef first  = loadFilter(stream);
    FilterRef second = loadFilter(stream);

    return combineFilters(first, second);
}


inline
HardFilter::FilterRef combineFilters(const HardFilter::FilterRef& first,
                                     const HardFilter::FilterRef& second)
{
    if (first->isNull())
    {
        if (second->isNull())
            return HardFilter::FilterRef(new NullFilter);
        else
            return second;
    }
    else
    {
        if (second->isNull())
            return first;
        else
            return HardFilter::FilterRef(new CombinedFilter(first, second));
    }
}


}


#endif // FFS_FILTER_H_INCLUDED

