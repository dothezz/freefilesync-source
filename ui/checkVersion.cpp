// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#include "checkVersion.h"
#include <wx/msgdlg.h>
#include <wx/protocol/http.h>
#include <wx/sstream.h>
#include "../version/version.h"
#include <wx/utils.h>
#include <wx/timer.h>
#include "../shared/globalFunctions.h"
#include "messagePopup.h"
#include "../shared/standardPaths.h"


class CloseConnectionOnExit
{
public:
    CloseConnectionOnExit(wxInputStream* httpStream, wxHTTP& webAccess) :
        m_httpStream(httpStream),
        m_webAccess(webAccess) {}

    ~CloseConnectionOnExit()
    {
        delete m_httpStream; //must be deleted BEFORE webAccess is closed
        m_webAccess.Close();
    }

private:
    wxInputStream* m_httpStream;
    wxHTTP& m_webAccess;
};


bool getOnlineVersion(wxString& version)
{
    wxHTTP webAccess;
    wxInputStream* httpStream = NULL;

    wxWindowDisabler dummy;
    CloseConnectionOnExit dummy2(httpStream, webAccess);

    webAccess.SetHeader(wxT("Content-type"), wxT("text/html; charset=utf-8"));
    webAccess.SetTimeout(5); //5 seconds of timeout instead of 10 minutes...

    if (webAccess.Connect(wxT("freefilesync.cvs.sourceforge.net"))) //only the server, no pages here yet...
    {
        //wxApp::IsMainLoopRunning(); // should return true

        httpStream = webAccess.GetInputStream(wxT("/viewvc/freefilesync/version/version.txt"));

        if (httpStream && webAccess.GetError() == wxPROTO_NOERR)
        {
            wxString newestVersion;
            wxStringOutputStream out_stream(&newestVersion);
            httpStream->Read(out_stream);
            if (!newestVersion.empty())
            {
                version = newestVersion;
                return true;
            }
        }
    }

    return false;
}


bool newerVersionExists(const wxString& onlineVersion)
{
    wxString currentVersionCpy = FreeFileSync::currentVersion;
    wxString onlineVersionCpy  = onlineVersion;

    const wxChar VERSION_SEP = wxT('.');

    using globalFunctions::stringToNumber;

    while ( currentVersionCpy.Find(VERSION_SEP) != wxNOT_FOUND ||
            onlineVersionCpy.Find(VERSION_SEP) != wxNOT_FOUND)
    {
        const int currentMajor = stringToNumber<int>(currentVersionCpy.BeforeFirst(VERSION_SEP));  //Returns the whole string if VERSION_SEP is not found.
        const int onlineMajor  = stringToNumber<int>(onlineVersionCpy.BeforeFirst(VERSION_SEP));   //Returns the whole string if VERSION_SEP is not found.

        if (currentMajor != onlineMajor)
            return currentMajor < onlineMajor;

        currentVersionCpy = currentVersionCpy.AfterFirst(VERSION_SEP); //Returns the empty string if VERSION_SEP is not found.
        onlineVersionCpy  = onlineVersionCpy.AfterFirst(VERSION_SEP);  //Returns the empty string if VERSION_SEP is not found.
    }

    return stringToNumber<int>(currentVersionCpy) < stringToNumber<int>(onlineVersionCpy);
}


void FreeFileSync::checkForUpdateNow()
{
    wxString onlineVersion;
    if (!getOnlineVersion(onlineVersion))
    {
        wxMessageBox(_("Unable to connect to sourceforge.net!"), _("Error"), wxOK | wxICON_ERROR);
        return;
    }

    if (newerVersionExists(onlineVersion))
    {
        const int rv = wxMessageBox(wxString(_("A newer version of FreeFileSync is available:"))  + wxT(" v") + onlineVersion + wxT(". ") + _("Download now?"), _("Information"), wxYES_NO | wxICON_QUESTION);
        if (rv == wxYES)
            wxLaunchDefaultBrowser(wxT("http://sourceforge.net/project/showfiles.php?group_id=234430"));
    }
    else
        wxMessageBox(_("FreeFileSync is up to date!"), _("Information"), wxICON_INFORMATION);
}


void FreeFileSync::checkForUpdatePeriodically(long& lastUpdateCheck)
{
#ifdef FFS_LINUX
    if (!FreeFileSync::isPortableVersion()) //don't check for updates in installer version -> else: handled by .deb
        return;
#endif

    if (lastUpdateCheck != -1)
    {
        if (lastUpdateCheck == 0)
        {
            QuestionDlg* const messageDlg = new QuestionDlg(NULL,
                    QuestionDlg::BUTTON_YES | QuestionDlg::BUTTON_CANCEL,
                    wxString(_("Do you want FreeFileSync to automatically check for updates every week?")) + wxT("\n") +
                    _("(Requires an Internet connection!)"));

            const bool checkRegularly = messageDlg->ShowModal() == QuestionDlg::BUTTON_YES;
            messageDlg->Destroy();
            if (checkRegularly)
            {
                lastUpdateCheck = 123; //some old date (few seconds after 1970)

                checkForUpdatePeriodically(lastUpdateCheck); //check for updates now
            }
            else
                lastUpdateCheck = -1; //don't check for updates anymore
        }
        else if (wxGetLocalTime() >= lastUpdateCheck + 7 * 24 * 3600) //check weekly
        {
            wxString onlineVersion;
            if (!getOnlineVersion(onlineVersion))
                return; //do not handle error

            lastUpdateCheck = wxGetLocalTime();

            if (newerVersionExists(onlineVersion))
            {
                const int rv = wxMessageBox(wxString(_("A newer version of FreeFileSync is available:"))  + wxT(" v") + onlineVersion + wxT(". ") + _("Download now?"), _("Information"), wxYES_NO | wxICON_QUESTION);
                if (rv == wxYES)
                    wxLaunchDefaultBrowser(wxT("http://sourceforge.net/project/showfiles.php?group_id=234430"));
            }
        }
    }
}

