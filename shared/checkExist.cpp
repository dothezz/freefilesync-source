#include "checkExist.h"
#include "parallelCall.h"
#include "fileHandling.h"
  
   
namespace
{
template <bool (*testExist)(const Zstring&)>
class CheckObjectExists : public Async::Procedure
{ 
public:
    CheckObjectExists(const Zstring& filename) :
        filename_(filename.c_str()), //deep copy: worker thread may run longer than main! Avoid shared data
        isExisting(false) {}

    virtual void doWork()
    {
        isExisting = testExist(filename_); //throw()
    }

    bool doesExist() const //retrieve result
    {
        return isExisting;
    }

private:
    const Zstring filename_; //no reference, lifetime not known
    bool isExisting;
};


template <bool (*testExist)(const Zstring&)>
inline
Utility::ResultExist objExists(const Zstring& filename, size_t timeout) //timeout in ms
{
    typedef CheckObjectExists<testExist> CheckObjEx;
    boost::shared_ptr<CheckObjEx> proc(new CheckObjEx(filename));

    return Async::execute(proc, timeout) == Async::TIMEOUT ? Utility::EXISTING_TIMEOUT :
           (proc->doesExist() ? Utility::EXISTING_TRUE : Utility::EXISTING_FALSE);
}
}


Utility::ResultExist Utility::fileExists(const Zstring& filename, size_t timeout) //timeout in ms
{
    return objExists<FreeFileSync::fileExists>(filename, timeout);
}


Utility::ResultExist Utility::dirExists(const Zstring& dirname, size_t timeout) //timeout in ms
{
    return objExists<FreeFileSync::dirExists>(dirname, timeout);
}
