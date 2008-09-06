/***************************************************************
 * Name:      FreeFileSyncApp.h
 * Purpose:   Defines Application Class
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2008-07-16
 * Copyright: ZenJu ()
 * License:
 **************************************************************/

#ifndef FREEFILESYNCAPP_H
#define FREEFILESYNCAPP_H

#include <wx/app.h>
#include <wx/cmdline.h>
#include <set>
#include <fstream>
#include "FreeFileSync.h"
#include "ui/smallDialogs.h"

struct TranslationLine
{
    wxString original;
    wxString translation;

    bool operator>(const TranslationLine& b ) const
    {
        return (original > b.original);
    }
    bool operator<(const TranslationLine& b) const
    {
        return (original < b.original);
    }
    bool operator==(const TranslationLine& b) const
    {
        return (original == b.original);
    }
};

typedef set<TranslationLine> Translation;


class CustomLocale : public wxLocale
{
public:
    CustomLocale(int language = wxLANGUAGE_ENGLISH,
                 int flags = wxLOCALE_LOAD_DEFAULT | wxLOCALE_CONV_ENCODING);

    ~CustomLocale() {}

    const wxChar* GetString(const wxChar* szOrigString, const wxChar* szDomain = NULL) const;

private:
    Translation translationDB;
};


class Application : public wxApp
{
public:
    bool OnInit();
    int  OnRun();
    int  OnExit();

    void initialize();
    bool ProcessIdle(); //virtual method

    friend class CommandLineStatusUpdate;

    //methods for writing logs
    void initLog();
    void writeLog(const wxString& logText, const wxString& problemType = wxEmptyString);
    void closeLog();

private:
    void parseCommandline();

    bool applicationRunsOnCommandLineWithoutWindows;

    ofstream logFile;
    CustomLocale* programLanguage;

    int returnValue;
};


class CommandLineStatusUpdater : public StatusUpdater
{
public:
    CommandLineStatusUpdater(Application* application, bool skipErr, bool silent);
    ~CommandLineStatusUpdater();

    void updateStatus(const wxString& text);
    void updateProgressIndicator(double number);
    int reportError(const wxString& text);

    void triggerUI_Refresh();

    void switchToSyncProcess(double number);
    void updateFinalStatus(const wxString& text);

private:
    Application* app;
    SyncStatus* syncStatusFrame;
    bool skipErrors;
    bool silentMode;

    bool switchedToSynchronisation;
    wxArrayString unhandledErrors;   //list of non-resolved errors
};


#endif // FREEFILESYNCAPP_H
