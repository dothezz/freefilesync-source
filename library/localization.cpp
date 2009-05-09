#include "localization.h"
#include <wx/msgdlg.h>
#include "resources.h"
#include "globalFunctions.h"
#include <fstream>
#include <set>

//_("Browse") <- dummy string for wxDirPickerCtrl to be recognized by automatic text extraction!


struct TranslationLine
{
    wxString original;
    wxString translation;

    bool operator<(const TranslationLine& b) const
    {
        return (original < b.original);
    }
};

class Translation : public std::set<TranslationLine> {};


CustomLocale::CustomLocale() :
        wxLocale(),
        currentLanguage(wxLANGUAGE_ENGLISH)
{
    translationDB = new Translation;
}


CustomLocale::~CustomLocale()
{
    delete translationDB;
}


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
            if ((value = *input) == wxChar(0))
                break;

            switch (value)
            {
            case wxChar('\\'):
                output += wxChar('\\');
                break;
            case wxChar('n'):
                output += wxChar('\n');
                break;
            case wxChar('t'):
                output += wxChar('\t');
                break;
            case wxChar('\"'):
                output += wxChar('\"');
                break;
            default:
                output += value;
            }
        }
        else
            output += value;

        ++input;
    }
    data = output;
}


void CustomLocale::setLanguage(const int language)
{
    currentLanguage = language;

    std::string languageFile;
    switch (language)
    {
    case wxLANGUAGE_CHINESE_SIMPLIFIED:
        languageFile = "Languages/chinese_simple.lng";
        break;
    case wxLANGUAGE_DUTCH:
        languageFile = "Languages/dutch.lng";
        break;
    case wxLANGUAGE_FRENCH:
        languageFile = "Languages/french.lng";
        break;
    case wxLANGUAGE_GERMAN:
        languageFile = "Languages/german.lng";
        break;
    case wxLANGUAGE_HUNGARIAN:
        languageFile = "Languages/hungarian.lng";
        break;
    case wxLANGUAGE_ITALIAN:
        languageFile = "Languages/italian.lng";
        break;
    case wxLANGUAGE_JAPANESE:
        languageFile = "Languages/japanese.lng";
        break;
    case wxLANGUAGE_POLISH:
        languageFile = "Languages/polish.lng";
        break;
    case wxLANGUAGE_PORTUGUESE:
        languageFile = "Languages/portuguese.lng";
        break;
    case wxLANGUAGE_PORTUGUESE_BRAZILIAN:
        languageFile = "Languages/portuguese_br.lng";
        break;
    case wxLANGUAGE_SLOVENIAN:
        languageFile = "Languages/slovenian.lng";
        break;
    case wxLANGUAGE_SPANISH:
        languageFile = "Languages/spanish.lng";
        break;
    default:
        languageFile.clear();
        currentLanguage = wxLANGUAGE_ENGLISH;
    }

    static bool initialized = false; //wxLocale is a global too!
    if (!initialized)
    {
        initialized = true;
        wxLocale::Init(currentLanguage, wxLOCALE_LOAD_DEFAULT | wxLOCALE_CONV_ENCODING);
    }

    //load language file into buffer
    translationDB->clear();
    const int bufferSize = 100000;
    char temp[bufferSize];
    if (!languageFile.empty())
    {
        std::ifstream langFile(languageFile.c_str(), std::ios::binary);
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
                    translationDB->insert(currentLine);
                }
            }
            langFile.close();
        }
        else
            wxMessageBox(wxString(_("Error reading file:")) + wxT(" \"") + wxString(languageFile.c_str(), wxConvUTF8) + wxT("\""), _("An exception occured!"), wxOK | wxICON_ERROR);
    }
    else
        ;   //if languageFile is empty texts will be english per default

    //these global variables need to be redetermined on language selection
    GlobalResources::DECIMAL_POINT = _(".");
    GlobalResources::THOUSANDS_SEPARATOR = _(",");
}


const wxChar* CustomLocale::GetString(const wxChar* szOrigString, const wxChar* szDomain) const
{
    TranslationLine currentLine;
    currentLine.original = szOrigString;

    //look for translation in buffer table
    const Translation::iterator i = translationDB->find(currentLine);
    if (i != translationDB->end())
        return i->translation.c_str();

    //fallback
    return szOrigString;
}
