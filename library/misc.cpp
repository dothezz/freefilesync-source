#include "misc.h"
#include <fstream>
#include <wx/msgdlg.h>
#include "resources.h"

void exchangeEscapeChars(wxString& data)
{
    data.Replace(wxT("\\\\"), wxT("\\"));
    data.Replace(wxT("\\n"), wxT("\n"));
    data.Replace(wxT("\\t"), wxT("\t"));
    data.Replace(wxT("\"\""), wxT(""""));
    data.Replace(wxT("\\\""), wxT("\""));
}


CustomLocale::CustomLocale() :
        wxLocale(),
        currentLanguage(wxLANGUAGE_ENGLISH)
{}


CustomLocale::~CustomLocale()
{
    //write language to file
    ofstream output("lang.dat");
    if (output)
    {
        output<<currentLanguage<<char(0);
        output.close();
    }
    else
        wxMessageBox(wxString(_("Could not write to ")) + wxT("\"") + wxT("lang.dat") + wxT("\""), _("An exception occured!"), wxOK | wxICON_ERROR);
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

    string languageFile;
    switch (language)
    {
    case wxLANGUAGE_GERMAN:
        languageFile = "german.lng";
        break;

    default:
        languageFile = string();
        currentLanguage = wxLANGUAGE_ENGLISH;
    }

    //load language file into buffer
    translationDB.clear();
    const int bufferSize = 100000;
    char temp[bufferSize];
    if (!languageFile.empty())
    {
        ifstream langFile(languageFile.c_str(), ios::binary);
        if (langFile)
        {
            int rowNumber = 0;
            TranslationLine currentLine;

            //Delimiter:
            //----------
            //Linux: 0xa
            //Mac:   0xd
            //Win:   0xd 0xa
            while (langFile.getline(temp, bufferSize, 0xd)) //specify delimiter explicitely
            {
                langFile.get(); //discard the 0xa character

                //wxString formattedString(temp, *wxConvUTF8, bufferSize); //convert UTF8 input to Unicode
                wxString formattedString(temp);

                exchangeEscapeChars(formattedString);

                if (rowNumber%2 == 0)
                    currentLine.original = formattedString;
                else
                {
                    currentLine.translation = formattedString;
                    translationDB.insert(currentLine);
                }
                ++rowNumber;
            }
            langFile.close();
        }
        else
            wxMessageBox(wxString(_("Could not read language file ")) + wxT("\"") + wxString(languageFile.c_str(), wxConvUTF8) + wxT("\""), _("An exception occured!"), wxOK | wxICON_ERROR);
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

