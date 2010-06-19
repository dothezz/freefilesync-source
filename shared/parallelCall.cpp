// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "parallelCall.h"
#include <wx/thread.h>
#include <stdexcept>
#include <wx/string.h>
#include <wx/msgdlg.h>

namespace
{
class WorkerThread : public wxThread
{
public:
    WorkerThread(const Async::AsyncProcess& proc,
                 const boost::shared_ptr<wxMutex>& mainIsListening,
                 const boost::shared_ptr<wxCondition>& procHasFinished) :
        wxThread(wxTHREAD_DETACHED),
        proc_(proc),                       //copy input data by value
        mainIsListening_(mainIsListening), //
        procHasFinished_(procHasFinished)  //
    {
        if (Create() != wxTHREAD_NO_ERROR)
            throw std::runtime_error("Error creating async worker thread!");

        if (Run() != wxTHREAD_NO_ERROR)
            throw std::runtime_error("Error starting async worker thread!");
    }

    ExitCode Entry()
    {
        try
        {
            proc_->doWork();

            //notify that work is done
            wxMutexLocker dummy(*mainIsListening_);
            procHasFinished_->Signal();
        }
        catch (const std::exception& e) //exceptions must be catched per thread
        {
            wxMessageBox(wxString::FromAscii(e.what()), _("An exception occured!"), wxOK | wxICON_ERROR);
        }
        catch (...) //exceptions must be catched per thread
        {
            wxMessageBox(wxT("Unknown exception in worker thread"), _("An exception occured!"), wxOK | wxICON_ERROR);
        }

        return 0;
    }

private:
    Async::AsyncProcess proc_;
    boost::shared_ptr<wxMutex> mainIsListening_;      //shared pointer is safe to use in MT context (same guarantee like builtin types!)
    boost::shared_ptr<wxCondition> procHasFinished_;  //http://www.boost.org/doc/libs/1_43_0/libs/smart_ptr/shared_ptr.htm#ThreadSafety
};
}


Async::Result Async::execute(AsyncProcess proc, size_t maxWait) //maxWait = max. wait time in milliseconds
{
    boost::shared_ptr<wxMutex> mainIsListening(new wxMutex);
    boost::shared_ptr<wxCondition> procHasFinished(new wxCondition(*mainIsListening));
    wxMutexLocker dummy(*mainIsListening); //the mutex should be initially locked (= "main not listening")

    new WorkerThread(proc, mainIsListening, procHasFinished);

    return procHasFinished->WaitTimeout(static_cast<unsigned long>(maxWait)) == wxCOND_NO_ERROR ? WORK_DONE : TIMEOUT;
}


//    ------------------------------------------------------
//    |Pattern: workload queue and multiple worker threads |
//    ------------------------------------------------------
//typedef std::vector<DirectoryDescrType*> Workload;
//
//class ThreadSorting : public wxThread
//{
//public:
//    ThreadSorting(wxCriticalSection& syncWorkload, Workload& workload) :
//            wxThread(wxTHREAD_JOINABLE),
//            syncWorkload_(syncWorkload),
//            workload_(workload)
//    {
//        if (Create() != wxTHREAD_NO_ERROR)
//            throw RuntimeException(wxString(wxT("Error creating thread for sorting!")));
//    }
//
//    ~ThreadSorting() {}
//
//
//    ExitCode Entry()
//    {
//        while (true)
//        {
//            DirectoryDescrType* descr = NULL;
//            {  //see if there is work to do...
//                wxCriticalSectionLocker dummy(syncWorkload_);
//                if (workload_.empty())
//                    return 0;
//                else
//                {
//                    descr = workload_.back();
//                    workload_.pop_back();
//                }
//            }
//            //do work
//            std::sort(descr->begin(), descr->end());
//        }
//    }
//
//private:
//    wxCriticalSection& syncWorkload_;
//    Workload& workload_;
//};
//
//
//void DirectoryDescrBuffer::preFillBuffers(const std::vector<FolderPairCfg>& fpConfigFormatted)
//{
//    //assemble workload
//	...
//
//    //we use binary search when comparing the directory structures: so sort() first
//    const int CPUCount = wxThread::GetCPUCount();
//    if (CPUCount >= 2) //do it the multithreaded way:
//    {
//        wxCriticalSection syncWorkload;
//
//        typedef std::vector<boost::shared_ptr<ThreadSorting> > ThreadContainer;
//        ThreadContainer sortThreads;
//        sortThreads.reserve(CPUCount);
//
//        //start CPUCount worker threads
//        for (size_t i = 0; i < std::min(static_cast<size_t>(CPUCount), workload.size()); ++i)
//        {
//            boost::shared_ptr<ThreadSorting> newWorker(new ThreadSorting(syncWorkload, workload));
//
//            if (newWorker->Run() != wxTHREAD_NO_ERROR)
//                throw RuntimeException(wxString(wxT("Error starting thread for sorting!")));
//
//            sortThreads.push_back(newWorker);
//        }
//
//        //wait until all worker are finished
//        for (ThreadContainer::iterator i = sortThreads.begin(); i != sortThreads.end(); ++i)
//        {
//            if ((*i)->Wait() != 0)
//                throw RuntimeException(wxString(wxT("Error waiting for thread (sorting)!")));
//        }
//    }
//    else //single threaded
//    {
//        for (Workload::iterator i = workload.begin(); i != workload.end(); ++i)
//            std::sort((*i)->begin(), (*i)->end());
//    }
//}
