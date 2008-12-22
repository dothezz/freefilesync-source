#include "misc.h"
#include <wx/msgdlg.h>
#include "resources.h"
#include "globalFunctions.h"


inline
void exchangeEscapeChars(wxString& data)
{
    wxString output;

    const wxChar* input = data.c_str();

    wxChar value;
    while ((value = *input) != wxChar(0))
    {
        //read backslash
        if (value == wxChar('\\'))
        {
            //read next character
            ++input;
            value = *input;
            if (value == wxChar(0))
                break;

            switch (value)
            {
            case wxChar('\\'):
                output+= wxChar('\\');
                break;
            case wxChar('n'):
                output+= wxChar('\n');
                break;
            case wxChar('t'):
                output+= wxChar('\t');
                break;
            case wxChar('\"'):
                output+= wxChar('\"');
                break;
            default:
                output+= value;
            }
        }
        else
            output+= value;

        ++input;
    }
    data = output;
}


CustomLocale::CustomLocale() :
        wxLocale(),
        currentLanguage(wxLANGUAGE_ENGLISH)
{}


void CustomLocale::setLanguage(const int language)
{
    currentLanguage = language;

    string languageFile;
    switch (language)
    {
    case wxLANGUAGE_GERMAN:
        languageFile = "german.lng";
        break;
    case wxLANGUAGE_FRENCH:
        languageFile = "french.lng";
        break;
    case wxLANGUAGE_JAPANESE:
        languageFile = "japanese.lng";
        break;
    case wxLANGUAGE_DUTCH:
        languageFile = "dutch.lng";
        break;
    default:
        languageFile.clear();
        currentLanguage = wxLANGUAGE_ENGLISH;
    }

    static bool initialized = false; //wxLocale is a "static" too!
    if (!initialized)
    {
        initialized = true;
        wxLocale::Init(currentLanguage, wxLOCALE_LOAD_DEFAULT | wxLOCALE_CONV_ENCODING);
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
            TranslationLine currentLine;

            //Delimiter:
            //----------
            //Linux: 0xa        \n
            //Mac:   0xd        \r
            //Win:   0xd 0xa    \r\n    <- language files are in Windows format
            for (int rowNumber = 0; langFile.getline(temp, bufferSize, 0xd); ++rowNumber) //specify delimiter explicitly
            {
                langFile.get(); //discard the 0xa character

                wxString formattedString = wxString::FromUTF8(temp);
                //wxString formattedString = wxString::From8BitData(temp);

                exchangeEscapeChars(formattedString);

                if (rowNumber%2 == 0)
                    currentLine.original = formattedString;
                else
                {
                    currentLine.translation = formattedString;
                    translationDB.insert(currentLine);
                }
            }
            langFile.close();
        }
        else
            wxMessageBox(wxString(_("Could not read language file ")) + wxT("\"") + wxString(languageFile.c_str(), wxConvUTF8) + wxT("\""), _("An exception occured!"), wxOK | wxICON_ERROR);
    }
    else
        ;   //if languageFile is empty texts will be english per default

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
