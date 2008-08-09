/***************************************************************
 * Name:      FreeFileSyncApp.cpp
 * Purpose:   Code for Application Class
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2008-07-16
 * Copyright: ZenJu ()
 * License:
 **************************************************************/

#include "Application.h"
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <stdexcept> //for std::runtime_error
#include "FreeFileSync.h"
//#include <wx/datetime.h>

IMPLEMENT_APP(Application);

bool Application::OnInit()
{
    try
    {   //set working directory to current executable directory
        if (!wxSetWorkingDirectory(wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath()))
            throw runtime_error(_("Could not set working directory to directory containing executable file!"));

        //set program language
            programLanguage = new CustomLocale();
//        switch (wxLocale::GetSystemLanguage())
//        {
//        case wxLANGUAGE_GERMAN:
//        case wxLANGUAGE_GERMAN_AUSTRIAN:
//        case wxLANGUAGE_GERMAN_BELGIUM:
//        case wxLANGUAGE_GERMAN_LIECHTENSTEIN:
//        case wxLANGUAGE_GERMAN_LUXEMBOURG:
//        case wxLANGUAGE_GERMAN_SWISS:
//            programLanguage = new CustomLocale(wxLANGUAGE_GERMAN);
//            break;
//        default:
//            programLanguage = new CustomLocale(wxLANGUAGE_ENGLISH);
//            break;
//        }

        //do not use wxApp::OnInit() to parse commandline but use own logic instead
        if (parsedCommandline())   //Note: "return false" is a must here since program crashes if "return true" and no windows have been created
            return false;

        //activate support for .png files
        wxImage::AddHandler(new wxPNGHandler);

        //show UI dialog
        MainDialog* frame = new MainDialog(0L);
        frame->SetIcon(wxICON(aaaa)); // To Set App Icon

        frame->Show();
    }
    catch (std::runtime_error& theException)
    {
        wxMessageBox(_(theException.what()), _("An exception occured!"), wxOK | wxICON_ERROR);
    }
    return true;
}

int Application::OnExit()
{
    delete programLanguage;
    return 0;
}


SyncDirection convertCmdlineCfg(const wxString& cfg, const int i)
{
    assert (cfg.Len() == 5);
    assert (0 <= i <= 4);

    switch (cfg[i])
    {
    case 'L':
        return syncDirLeft;
        break;
    case 'R':
        return syncDirRight;
        break;
    case 'N':
        return syncDirNone;
        break;
    default:
        assert(false);
    }
}

void writeLog(const wxString& logText, const wxString& problemType, bool silentMode)
{
    if (silentMode)
    {
        wxString tmp = wxDateTime::Now().FormatISOTime();
        tmp.Replace(":", "");
        wxString logfileName = wxString("FFS_") + wxDateTime::Now().FormatISODate() + '_' + tmp + ".log";
        ofstream output(logfileName.c_str());
        if (!output)
            throw std::runtime_error(_("Unable to create logfile!"));

        output<<_("FreeFileSync   (Date: ") + wxDateTime::Now().FormatDate() + _("  Time: ") +  wxDateTime::Now().FormatTime() + ")"<<endl;
        output<<_("-------------------------------------------------")<<endl;
        output<<endl;
        output<<_("Log-message:")<<endl;
        output<<endl;
        output<<problemType<<": "<<logText<<endl;

        output.close();
    }
    else
        wxMessageBox(logText, problemType, wxICON_WARNING);
}


bool Application::parsedCommandline()
{
    //commandline-descriptor must be initialized here AFTER the program language is set
    const wxCmdLineEntryDesc cmdLineDesc [] =
    {
        { wxCMD_LINE_SWITCH,
            wxT("h"),
            wxT("help"),
            wxT(_("displays help on the command line parameters")),
            wxCMD_LINE_VAL_NONE,
            wxCMD_LINE_OPTION_HELP
        },

        {
            wxCMD_LINE_OPTION,
            "cmp",
            NULL,
            _("Specify algorithm to test if files are equal:\n\n\tSIZEDATE: check filesize and date\n\tCONTENT: check file content\n"),
            wxCMD_LINE_VAL_STRING,
            wxCMD_LINE_OPTION_MANDATORY
        },

        {
            wxCMD_LINE_OPTION,
            "cfg",
            NULL,
            _("Specify the sync-direction used for each type of file by a string of five chars:\n\n\tChar 1: Folders/files that exist on left side only\n\tChar 2: Folders/files that exist on right side only\n\tChar 3: Folders/files that exist on both sides, left one is newer\n\tChar 4: Folders/files that exist on both sides, right one is newer\n\tChar 5: Folders/files that exist on both sides and are different\n\n\tSync-direction: L: left, R: right, N: do nothing\n"),
            wxCMD_LINE_VAL_STRING,
            wxCMD_LINE_OPTION_MANDATORY
        },

        {
            wxCMD_LINE_PARAM,
            NULL,
            NULL,
            _("<left directory>"),
            wxCMD_LINE_VAL_STRING
        },

        {
            wxCMD_LINE_PARAM,
            NULL,
            NULL,
            _("<right directory>"),
            wxCMD_LINE_VAL_STRING
        },

        {
            wxCMD_LINE_SWITCH,
            "silent",
            NULL,
            _("Do not show UI messages but write to a logfile instead.\n\nExample:\n\n1.) FreeFileSync -cmp SIZEDATE -cfg RRRRR c:\\source c:\\target\n2.) FreeFileSync -cmp sizedate -cfg rlrln c:\\dir1 c:\\dir2\n\n1: Creates a mirror backup of the left directory.\n2: Synchronizes both directories simultaneously.\n\n")
        },

        {
            wxCMD_LINE_NONE
        }

    };


    if (argc <= 1)
        return false;    //no parameters passed, continue with UI

    //now set the parser with all MANDATORY options and parameters
    wxCmdLineParser parser(cmdLineDesc, argc, argv);
    parser.SetSwitchChars(wxT("-"));
    if (parser.Parse() != 0)  //if commandline is used incorrectly display help dialog and exit program
        return true;

    //commandline parameters
    wxString cmp;
    wxString cfg;
    wxString leftDir;
    wxString rightDir;

//check existence of all commandline parameters
    if (!parser.Found("cmp", &cmp) ||
            !parser.Found("cfg", &cfg) ||
            parser.GetParamCount() != 2)
    {
        parser.Usage();
        return true;
    }

    cmp.UpperCase();
    cfg.UpperCase();
    leftDir = parser.GetParam(0);
    rightDir = parser.GetParam(1);

//until here all options and parameters had been set

//check consistency of all commandline parameters
    if ((cmp != "SIZEDATE" && cmp != "CONTENT") ||
            cfg.Len() != 5 ||
            (cfg[0] != 'L' && cfg[0] != 'R' && cfg[0] != 'N') ||
            (cfg[1] != 'L' && cfg[1] != 'R' && cfg[1] != 'N') ||
            (cfg[2] != 'L' && cfg[2] != 'R' && cfg[2] != 'N') ||
            (cfg[3] != 'L' && cfg[3] != 'R' && cfg[3] != 'N') ||
            (cfg[4] != 'L' && cfg[4] != 'R' && cfg[4] != 'N'))
    {
        parser.Usage();
        return true;
    }

    wxString logText;

    //check if directories exist
    if (!wxDirExists(leftDir))
    {
        writeLog(wxString(_("Directory ")) + leftDir + _(" does not exist. Aborting!"), _("Warning"), parser.Found("silent"));
        return true;
    }
    else if (!wxDirExists(rightDir))
    {
        writeLog(wxString(_("Directory ")) + rightDir + _(" does not exist. Aborting!"), _("Warning"), parser.Found("silent"));
        return true;
    }

//until here all options and parameters are consistent

    CompareVariant cmpVar;
    SyncConfiguration syncConfiguration;
    FileCompareResult currentGridData;

    if (cmp == "SIZEDATE")
        cmpVar = compareByTimeAndSize;
    else if (cmp == "CONTENT")
        cmpVar = compareByMD5;

    syncConfiguration.exLeftSideOnly  = convertCmdlineCfg(cfg, 0);
    syncConfiguration.exRightSideOnly = convertCmdlineCfg(cfg, 1);
    syncConfiguration.leftNewer       = convertCmdlineCfg(cfg, 2);
    syncConfiguration.rightNewer      = convertCmdlineCfg(cfg, 3);
    syncConfiguration.different       = convertCmdlineCfg(cfg, 4);

    try
    {
//COMPARE DIRECTORIES
        FreeFileSync::getModelDiff(currentGridData, leftDir, rightDir, cmpVar);

        //check if folders are already in sync
        bool nothingToSync = true;
        for (FileCompareResult::const_iterator i = currentGridData.begin(); i != currentGridData.end(); ++i)
            if (i->cmpResult != filesEqual)
            {
                nothingToSync = false;
                break;
            }

        if (nothingToSync)
        {
            writeLog(_("Nothing to synchronize. Both directories seem to contain the same data!"), _("Information"), parser.Found("silent"));
            return true;
        }

//SYNCHRONIZE DIRECTORIES
        FreeFileSync fileSyncObject;    //currently only needed for use of recycle bin

        //synchronize folders:
        for (FileCompareResult::const_iterator i = currentGridData.begin(); i != currentGridData.end(); ++i)
            if (i->fileDescrLeft.objType == isDirectory || i->fileDescrRight.objType == isDirectory)
                fileSyncObject.synchronizeFolder(*i, syncConfiguration);

        //synchronize files:
        for (FileCompareResult::const_iterator i = currentGridData.begin(); i !=currentGridData.end(); ++i)
            if (i->fileDescrLeft.objType == isFile || i->fileDescrRight.objType == isFile)
                fileSyncObject.synchronizeFile(*i, syncConfiguration);
    }
    catch (std::runtime_error& theException)
    {
        writeLog(theException.what(), _("An exception occured"), parser.Found("silent"));
        return true;
    }

// show success message
    writeLog(_("Synchronization completed successfully!"), _("Information"), parser.Found("silent"));
    return true;   //exit program and skip UI dialogs
}


wxString exchangeEscapeChars(char* temp)
{
    wxString output(temp);
    output.Replace("\\\\", "\\");
    output.Replace("\\n", "\n");
    output.Replace("\\t", "\t");
    output.Replace("\\\"", "\"");
    output.Replace("\"\"", """");
    return output;
}


CustomLocale::CustomLocale(int language, int flags)
        :wxLocale(language, flags)
{
    char temp[100000];
//    wxString languageFile;
//    switch (language)
//    {
//    case wxLANGUAGE_GERMAN:
      wxString languageFile = "language.dat";
//        break;
//    default: ;
//    }

    //load language file into buffer
    if (languageFile.size() != 0)
    {
        ifstream langFile(languageFile.c_str());
        if (langFile)
        {
            int rowNumber = 0;
            TranslationLine currentLine;
            while (langFile.getline(temp, 100000))
            {
                if (rowNumber%2 == 0)
                    currentLine.original = exchangeEscapeChars(temp);
                else
                {
                    currentLine.translation = exchangeEscapeChars(temp);
                    translationDB.insert(currentLine);
                }
                rowNumber++;
            }
            langFile.close();
        }
    }
}


const wxChar* CustomLocale::GetString(const wxChar* szOrigString, const wxChar* szDomain) const
{
    TranslationLine currentLine;
    currentLine.original = szOrigString;

    //look for tranlation in buffertable
    Translation::iterator i;
    if ((i = translationDB.find(currentLine)) != translationDB.end())
        return (i->translation.c_str());
    //fallback
    return (szOrigString);
}

