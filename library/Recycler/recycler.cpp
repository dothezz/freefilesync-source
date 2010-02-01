#include "recycler.h"

#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include <Shellapi.h>     // Included for shell constants such as FO_* values
#include <shobjidl.h>     // Required for necessary shell dependencies

#include <algorithm>
#include <string>
#include <cstdio>
#include <comdef.h>


void writeString(const wchar_t* input, wchar_t* output, size_t outputBufferLen)
{
    const size_t newSize = min(wcslen(input) + 1, outputBufferLen); //including null-termination
    memcpy(output, input, newSize * sizeof(wchar_t));
    output[newSize-1] = 0; //if output buffer is too small...
}


std::wstring numberToHexString(const long number)
{
    wchar_t result[100];
    swprintf(result, 100, L"0x%08x", number);
    return std::wstring(result);
}


void writeErrorMsg(const wchar_t* input, HRESULT hr, wchar_t* output, size_t outputBufferLen)
{
    std::wstring formattedMsg(input);
    formattedMsg += L" (";
    formattedMsg += numberToHexString(hr);
    formattedMsg += L": ";
    formattedMsg += _com_error(hr).ErrorMessage();
    formattedMsg += L")";

    writeString(formattedMsg.c_str(), output, outputBufferLen);
}


//IShellItem resource management
template <class T>
class ReleaseAtExit
{
public:
    ReleaseAtExit(T*& item) : item_(item) {}
    ~ReleaseAtExit()
    {
        if (item_ != NULL)
            item_->Release();
    }
private:
    T*& item_;
};


bool Utility::moveToRecycleBin(const wchar_t* fileNames[],
                               size_t         fileNo, //size of fileNames array
                               wchar_t*		  errorMessage,
                               size_t         errorBufferLen)
{
    HRESULT hr;

    // Create the IFileOperation interface
    IFileOperation* pfo = NULL;
    ReleaseAtExit<IFileOperation> dummy(pfo);
    hr = CoCreateInstance(CLSID_FileOperation,
                          NULL,
                          CLSCTX_ALL,
                          IID_PPV_ARGS(&pfo));
    if (FAILED(hr))
    {
        writeErrorMsg(L"Error calling \"CoCreateInstance\".", hr, errorMessage, errorBufferLen);
        return false;
    }

    // Set the operation flags.  Turn off  all UI
    // from being shown to the user during the
    // operation.  This includes error, confirmation
    // and progress dialogs.
    hr = pfo->SetOperationFlags(FOF_ALLOWUNDO |
                                FOF_NOCONFIRMATION |
                                FOF_SILENT |
                                FOF_NOERRORUI);
    if (FAILED(hr))
    {
        writeErrorMsg(L"Error calling \"SetOperationFlags\".", hr, errorMessage, errorBufferLen);
        return false;
    }

    for (size_t i = 0; i < fileNo; ++i)
    {
        //create file/folder item object
        IShellItem* psiFile = NULL;
        ReleaseAtExit<IShellItem> dummy2(psiFile);
        hr = SHCreateItemFromParsingName(fileNames[i],
                                         NULL,
                                         IID_PPV_ARGS(&psiFile));
        if (FAILED(hr))
        {
			    std::wstring message(L"Error calling \"SHCreateItemFromParsingName\" for file ");
				message += std::wstring(L"\"") + fileNames[i] + L"\".";
            writeErrorMsg(message.c_str(), hr, errorMessage, errorBufferLen);
            return false;
        }

        hr = pfo->DeleteItem(psiFile, NULL);
        if (FAILED(hr))
        {
            writeErrorMsg(L"Error calling \"DeleteItem\".", hr, errorMessage, errorBufferLen);
            return false;
        }
    }

    //perform actual operations
    hr = pfo->PerformOperations();
    if (FAILED(hr))
    {
        writeErrorMsg(L"Error calling \"PerformOperations\".", hr, errorMessage, errorBufferLen);
        return false;
    }

    //check if errors occured: if FOFX_EARLYFAILURE is not used, PerformOperations() can return with success despite errors!
    BOOL pfAnyOperationsAborted = FALSE;
    hr = pfo->GetAnyOperationsAborted(&pfAnyOperationsAborted);
    if (FAILED(hr))
    {
        writeErrorMsg(L"Error calling \"GetAnyOperationsAborted\".", hr, errorMessage, errorBufferLen);
        return false;
    }


    if (pfAnyOperationsAborted == TRUE)
    {
        writeString(L"Operation did not complete successfully.", errorMessage, errorBufferLen);
        return false;
    }

    return true;
}
