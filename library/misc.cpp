#include "misc.h"
#include <fstream>
#include <wx/msgdlg.h>
#include "resources.h"

wxString exchangeEscapeChars(char* temp)
{
    wxString output(temp);
    output.Replace("\\\\", "\\");
    output.Replace("\\n", "\n");
    output.Replace("\\t", "\t");
    output.Replace("\"\"", """");
    output.Replace("\\\"", "\"");
    return output;
}


CustomLocale::CustomLocale() :
        wxLocale(),
        currentLanguage(wxLANGUAGE_ENGLISH)
{}


CustomLocale::~CustomLocale()
{
    //write language to file
    ofstream output("lang.dat");
    if (!output)
    {
        wxMessageBox(wxString(_("Could not write to ")) + "\"" + "lang.dat" + "\"", _("An exception occured!"), wxOK | wxICON_ERROR);
        return;
    }
    output<<currentLanguage<<char(0);
    output.close();
}


void CustomLocale::loadLanguageFromCfg()  //retrieve language from config file: do NOT put into constructor since working directory has not been set!
{
    int language = wxLANGUAGE_ENGLISH;

    //try to load language setting from file
    ifstream input("lang.dat");
    if (input)
    {
        char buffer[20];
        input.getline(buffer, 20, char(0));
        language = atoi(buffer);
        input.close();
    }
    else
        language = wxLocale::GetSystemLanguage();

    wxLocale::Init(language, wxLOCALE_LOAD_DEFAULT | wxLOCALE_CONV_ENCODING);

    loadLanguageFile(language);
}


void CustomLocale::loadLanguageFile(int language)
{
    currentLanguage = language;

    wxString languageFile;
    switch (language)
    {
    case wxLANGUAGE_GERMAN:
        languageFile = "german.lng";
        break;

    default:
        languageFile = "";
        currentLanguage = wxLANGUAGE_ENGLISH;
    }

    //load language file into buffer
    translationDB.clear();
    char temp[100000];
    if (!languageFile.IsEmpty())
    {
        ifstream langFile(languageFile.c_str());
        if (langFile)
        {
            int rowNumber = 0;
            TranslationLine currentLine;

            //Delimiter:
            //----------
            //Linux: 0xa
            //Mac:   0xd
            //Win:   0xd 0xa
#ifdef FFS_WIN
            while (langFile.getline(temp, 100000))
            {
#elif defined FFS_LINUX
            while (langFile.getline(temp, 100000, 0xd)) //specify delimiter explicitely
            {
                //strangely this approach does NOT work under windows :(
                langFile.get(); //delimiter 0xd is completely ignored (and also not extracted)
#else
            assert (false);
#endif
                if (rowNumber%2 == 0)
                    currentLine.original = exchangeEscapeChars(temp);
                else
                {
                    currentLine.translation = exchangeEscapeChars(temp);
                    translationDB.insert(currentLine);
                }
                ++rowNumber;
            }
            langFile.close();
        }
        else
            wxMessageBox(wxString(_("Could not read language file ")) + "\"" + languageFile + "\"", _("An exception occured!"), wxOK | wxICON_ERROR);
    }
    else
        ;   //if languageFile is empty language is defaulted to english

    //these global variables need to be redetermined on language selection
    GlobalResources::decimalPoint = _(".");
    GlobalResources::thousandsSeparator = _(",");
}


const wxChar* CustomLocale::GetString(const wxChar* szOrigString, const wxChar* szDomain) const
{
    TranslationLine currentLine;
    currentLine.original = szOrigString;

    //look for translation in buffer table
    Translation::iterator i;
    if ((i = translationDB.find(currentLine)) != translationDB.end())
        return (i->translation.c_str());
    //fallback
    return (szOrigString);
}

