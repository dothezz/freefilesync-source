#include "localization.h"
#include <wx/msgdlg.h>
#include "../structures.h"
#include "globalFunctions.h"
#include <fstream>
#include <set>
#include <map>
#include "resources.h"

using FreeFileSync::CustomLocale;
using FreeFileSync::LocalizationInfo;

//_("Browse") <- dummy string for wxDirPickerCtrl to be recognized by automatic text extraction!


const std::vector<FreeFileSync::LocInfoLine>& LocalizationInfo::getMapping()
{
    static LocalizationInfo instance;
    return instance.locMapping;
}


LocalizationInfo::LocalizationInfo()
{
    FreeFileSync::LocInfoLine newEntry;

    newEntry.languageID     = wxLANGUAGE_GERMAN;
    newEntry.languageName   = wxT("Deutsch");
    newEntry.languageFile   = "Languages/german.lng";
    newEntry.translatorName = wxT("ZenJu");
    newEntry.languageFlag   = GlobalResources::getInstance().bitmapGermany;
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_ENGLISH;
    newEntry.languageName   = wxT("English");
    newEntry.languageFile   = "";
    newEntry.translatorName = wxT("ZenJu");
    newEntry.languageFlag   = GlobalResources::getInstance().bitmapEngland;
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_SPANISH;
    newEntry.languageName   = wxT("Español");
    newEntry.languageFile   = "Languages/spanish.lng";
    newEntry.translatorName = wxT("David Rodríguez");
    newEntry.languageFlag   = GlobalResources::getInstance().bitmapSpain;
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_FRENCH;
    newEntry.languageName   = wxT("Français");
    newEntry.languageFile   = "Languages/french.lng";
    newEntry.translatorName = wxT("Jean-François Hartmann");
    newEntry.languageFlag   = GlobalResources::getInstance().bitmapFrance;
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_ITALIAN;
    newEntry.languageName   = wxT("Italiano");
    newEntry.languageFile   = "Languages/italian.lng";
    newEntry.translatorName = wxT("Emmo");
    newEntry.languageFlag   = GlobalResources::getInstance().bitmapItaly;
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_HUNGARIAN;
    newEntry.languageName   = wxT("Magyar");
    newEntry.languageFile   = "Languages/hungarian.lng";
    newEntry.translatorName = wxT("Demon");
    newEntry.languageFlag   = GlobalResources::getInstance().bitmapHungary;
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_DUTCH;
    newEntry.languageName   = wxT("Nederlands");
    newEntry.languageFile   = "Languages/dutch.lng";
    newEntry.translatorName = wxT("M.D. Vrakking");
    newEntry.languageFlag   = GlobalResources::getInstance().bitmapHolland;
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_RUSSIAN;
    newEntry.languageName   = wxT("Pусский язык");
    newEntry.languageFile   = "Languages/russian.lng";
    newEntry.translatorName = wxT("Svobodniy");
    newEntry.languageFlag   = GlobalResources::getInstance().bitmapRussia;
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_POLISH;
    newEntry.languageName   = wxT("Polski");
    newEntry.languageFile   = "Languages/polish.lng";
    newEntry.translatorName = wxT("Wojtek Pietruszewski");
    newEntry.languageFlag   = GlobalResources::getInstance().bitmapPoland;
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_PORTUGUESE;
    newEntry.languageName   = wxT("Português");
    newEntry.languageFile   = "Languages/portuguese.lng";
    newEntry.translatorName = wxT("QuestMark");
    newEntry.languageFlag   = GlobalResources::getInstance().bitmapPortugal;
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_PORTUGUESE_BRAZILIAN;
    newEntry.languageName   = wxT("Português do Brasil");
    newEntry.languageFile   = "Languages/portuguese_br.lng";
    newEntry.translatorName = wxT("Edison Aranha");
    newEntry.languageFlag   = GlobalResources::getInstance().bitmapBrazil;
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_SLOVENIAN;
    newEntry.languageName   = wxT("Slovenščina");
    newEntry.languageFile   = "Languages/slovenian.lng";
    newEntry.translatorName = wxT("Matej Badalic");
    newEntry.languageFlag   = GlobalResources::getInstance().bitmapSlovakia;
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_JAPANESE;
    newEntry.languageName   = wxT("日本語");
    newEntry.languageFile   = "Languages/japanese.lng";
    newEntry.translatorName = wxT("Tilt");
    newEntry.languageFlag   = GlobalResources::getInstance().bitmapJapan;
    locMapping.push_back(newEntry);

    newEntry.languageID     = wxLANGUAGE_CHINESE_SIMPLIFIED;
    newEntry.languageName   = wxT("简体中文");
    newEntry.languageFile   = "Languages/chinese_simple.lng";
    newEntry.translatorName = wxT("Misty Wu");
    newEntry.languageFlag   = GlobalResources::getInstance().bitmapChina;
    locMapping.push_back(newEntry);
}



typedef wxString TextOriginal;
typedef wxString TextTranslation;

class Translation : public std::map<TextOriginal, TextTranslation> {};


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
    //default: english
    std::string languageFile;
    currentLanguage = wxLANGUAGE_ENGLISH;

    //(try to) retrieve language filename
    for (std::vector<LocInfoLine>::const_iterator i = LocalizationInfo::getMapping().begin(); i != LocalizationInfo::getMapping().end(); ++i)
        if (language == i->languageID)
        {
            languageFile    = i->languageFile;
            currentLanguage = i->languageID;
            break;
        }


    static bool initialized = false; //wxLocale is a static global too!
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
            wxString original;

            //Delimiter:
            //----------
            //Linux: 0xa        \n
            //Mac:   0xd        \r
            //Win:   0xd 0xa    \r\n    <- language files are in Windows format
            for (int rowNumber = 0; langFile.getline(temp, bufferSize, 0xD); ++rowNumber) //specify delimiter explicitly
            {
                langFile.get(); //discard the 0xa character

                wxString formattedString = wxString::FromUTF8(temp);
                //wxString formattedString = wxString::From8BitData(temp);

                exchangeEscapeChars(formattedString);

                if (rowNumber%2 == 0)
                    original = formattedString;
                else
                {
                    if (!formattedString.empty())
                    {
                        const wxString translation = formattedString;
                        translationDB->insert(std::pair<TextOriginal, TextTranslation>(original, translation));
                    }
                }
            }
            langFile.close();
        }
        else
            wxMessageBox(wxString(_("Error reading file:")) + wxT(" \"") + wxString(languageFile.c_str(), wxConvUTF8) + wxT("\""), _("Error"), wxOK | wxICON_ERROR);
    }
    else
        ;   //if languageFile is empty texts will be english per default

    //these global variables need to be redetermined on language selection
    FreeFileSync::DECIMAL_POINT = _(".");
    FreeFileSync::THOUSANDS_SEPARATOR = _(",");
}


const wxChar* CustomLocale::GetString(const wxChar* szOrigString, const wxChar* szDomain) const
{
    //look for translation in buffer table
    const Translation::const_iterator i = translationDB->find(szOrigString);
    if (i != translationDB->end())
        return i->second.c_str();

    //fallback
    return szOrigString;
}
