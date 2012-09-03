// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "check_version.h"
#include <memory>
#include <wx/msgdlg.h>
#include <wx/protocol/http.h>
#include <wx/sstream.h>
#include "../version/version.h"
#include <wx/utils.h>
#include <wx/timer.h>
#include <zen/string_tools.h>
#include "msg_popup.h"
#include "../lib/ffs_paths.h"
#include <zen/i18n.h>

using namespace zen;


wxString getOnlineVersion() //empty string on error;
{
    wxWindowDisabler dummy;

    wxHTTP webAccess;
    webAccess.SetHeader(L"Content-type", L"text/html; charset=utf-8");
    webAccess.SetTimeout(5); //5 seconds of timeout instead of 10 minutes(WTF are they thinking???)...

    if (webAccess.Connect(L"freefilesync.cvs.sourceforge.net")) //only the server, no pages here yet...
    {
        //wxApp::IsMainLoopRunning(); // should return true

        std::unique_ptr<wxInputStream> httpStream(webAccess.GetInputStream(L"/viewvc/freefilesync/version/version.txt"));
        //must be deleted BEFORE webAccess is closed

        if (httpStream && webAccess.GetError() == wxPROTO_NOERR)
        {
            wxString onlineVersion;
            wxStringOutputStream out_stream(&onlineVersion);
            httpStream->Read(out_stream);
            return onlineVersion;
        }
    }
    return wxString();
}


const wchar_t VERSION_SEP = L'.';

std::vector<size_t> parseVersion(const wxString& version)
{
    std::vector<wxString> digits = split(version, VERSION_SEP);

    std::vector<size_t> output;
    std::transform(digits.begin(), digits.end(), std::back_inserter(output), [&](const wxString& d) { return stringTo<size_t>(d); });
    return output;
}


bool isNewerVersion(const wxString& onlineVersion)
{
    std::vector<size_t> current = parseVersion(zen::currentVersion);
    std::vector<size_t> online  = parseVersion(onlineVersion);

    if (online.empty() || online[0] == 0) //online version may be "This website has been moved..." In this case better check for an update
        return true;

    return std::lexicographical_compare(current.begin(), current.end(),
                                        online .begin(), online .end());
}


void zen::checkForUpdateNow(wxWindow* parent)
{
    const wxString onlineVersion = getOnlineVersion();
    if (onlineVersion.empty())
    {
        wxMessageBox(_("Unable to connect to sourceforge.net!"), _("Error"), wxOK | wxICON_ERROR, parent);
        return;
    }

    if (isNewerVersion(onlineVersion))
    {
        if (showQuestionDlg(parent, ReturnQuestionDlg::BUTTON_YES | ReturnQuestionDlg::BUTTON_CANCEL,
                            _("A new version of FreeFileSync is available:")  + L" " + onlineVersion + L"\n\n" + _("Download now?")) == ReturnQuestionDlg::BUTTON_YES)
            wxLaunchDefaultBrowser(L"http://sourceforge.net/projects/freefilesync/files/freefilesync/v" + onlineVersion + L"/");
    }
    else
        wxMessageBox(_("FreeFileSync is up to date!"), _("Information"), wxICON_INFORMATION, parent);
}


void zen::checkForUpdatePeriodically(wxWindow* parent, long& lastUpdateCheck)
{
#ifdef FFS_LINUX
    if (!zen::isPortableVersion()) //don't check for updates in locally installed version -> handled by system updater
        return;
#endif

    if (lastUpdateCheck != -1)
    {
        if (lastUpdateCheck == 0)
        {
            switch (showQuestionDlg(parent, ReturnQuestionDlg::BUTTON_YES | ReturnQuestionDlg::BUTTON_NO | ReturnQuestionDlg::BUTTON_CANCEL,
                                    _("Do you want FreeFileSync to automatically check for updates every week?") + L"\n" +
                                    _("(Requires an Internet connection!)")))
            {
                case ReturnQuestionDlg::BUTTON_YES:
                    lastUpdateCheck = 123; //some old date (few seconds after 1970) different from 0 and -1
                    checkForUpdatePeriodically(parent, lastUpdateCheck); //check for updates now
                    break;

                case ReturnQuestionDlg::BUTTON_NO:
                    lastUpdateCheck = -1; //don't check for updates anymore
                    break;

                case ReturnQuestionDlg::BUTTON_CANCEL:
                    break;
            }
        }
        else if (wxGetLocalTime() >= lastUpdateCheck + 7 * 24 * 3600) //check weekly
        {
            const wxString onlineVersion = getOnlineVersion();
            if (onlineVersion.empty())
                return; //do not handle error

            lastUpdateCheck = wxGetLocalTime();

            if (isNewerVersion(onlineVersion))
            {
                if (showQuestionDlg(parent, ReturnQuestionDlg::BUTTON_YES | ReturnQuestionDlg::BUTTON_CANCEL,
                                    _("A new version of FreeFileSync is available:")  + L" " + onlineVersion + L"\n\n" + _("Download now?")) == ReturnQuestionDlg::BUTTON_YES)
                    wxLaunchDefaultBrowser(L"http://sourceforge.net/projects/freefilesync/files/freefilesync/v" + onlineVersion + L"/");
            }
        }
    }
}

