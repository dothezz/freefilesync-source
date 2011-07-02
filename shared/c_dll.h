// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef C_DLL_HEADER
#define C_DLL_HEADER

#include <string>
#include <map>
#include <algorithm>


namespace c_dll
{
void writeString(const std::wstring& input, wchar_t* output, size_t outputLen);


//Convert handles to objects and vice versa
template <class S, class T> //T: prefer managed object to ensure cleanup if remove() is not called
class HandleProvider
{
public:
    static HandleProvider& instance();
    S insert(T object);
    void remove(S handle);
    T& retrieve(S handle); //return default-constructed object if not found

private:
    HandleProvider() {}
    HandleProvider(const HandleProvider&);
    HandleProvider& operator=(const HandleProvider&);
    S generate();

    std::map<S, T> handleMap;
};
/*
Example:
		typedef HandleProvider<TBHandle, ComPtr<ITaskbarList3> > HandleTaskbarMap;
		HandleTaskbarMap::instance().insert(xyz);
*/




























//########################## inline implementation #############################
inline
void writeString(const std::wstring& input, wchar_t* output, size_t outputLen)
{
    if (outputLen > 0)
    {
        const size_t maxSize = std::min(input.length(), outputLen - 1);
        std::copy(input.begin(), input.begin() + maxSize, output);
        output[maxSize] = 0;
    }
}


template <class S, class T>
inline
HandleProvider<S, T>& HandleProvider<S, T>::instance()
{
    static HandleProvider inst; //external linkage!!! :)
    return inst;
}


//convert handles to objects and vice versa
template <class S, class T>
inline
S HandleProvider<S, T>::insert(T object)
{
    S newHandle = generate();
    handleMap.insert(std::make_pair(newHandle, object));
    return newHandle;
}


template <class S, class T>
inline
void HandleProvider<S, T>::remove(S handle)
{
    handleMap.erase(handle);
}


template <class S, class T>
inline
T& HandleProvider<S, T>::retrieve(S handle) //return default-constructed object if not found
{
    return handleMap[handle];
}


template <class S, class T>
inline
S HandleProvider<S, T>::generate()
{
    static S handle = 0;
    return ++handle; //don't return 0! 0 is reserved for indicating failure
}


}

#endif //C_DLL_HEADER