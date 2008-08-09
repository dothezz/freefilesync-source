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
#include "UI\MainDialog.h"
#include <wx/cmdline.h>
#include <set>
#include <fstream>

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
    int OnExit();
    bool parsedCommandline();

    CustomLocale* programLanguage;
};

#endif // FREEFILESYNCAPP_H
