// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef XMLBASE_H_INCLUDED
#define XMLBASE_H_INCLUDED

#include "tinyxml/tinyxml.h"
#include "globalFunctions.h"
#include <string>
#include <vector>
#include <wx/string.h>
#include "xmlError.h"


namespace xmlAccess
{

enum XmlType
{
    XML_GUI_CONFIG,
    XML_BATCH_CONFIG,
    XML_GLOBAL_SETTINGS,
    XML_REAL_CONFIG,
    XML_OTHER
};

XmlType getXmlType(const wxString& filename); //throw()

void loadXmlDocument(const wxString& fileName, const XmlType type, TiXmlDocument& document); //throw (XmlError)
void getDefaultXmlDocument(const XmlType type, TiXmlDocument& document);
void saveXmlDocument(const wxString& fileName, const TiXmlDocument& document); //throw (XmlError)


//------------------------------------------------------------------------------------------

//small helper functions
template <class T>
bool readXmlElement(const std::string& name, const TiXmlElement* parent, T&            output);
bool readXmlElement(const std::string& name, const TiXmlElement* parent, std::string&  output);
bool readXmlElement(const std::string& name, const TiXmlElement* parent, wxString&     output);
bool readXmlElement(const std::string& name, const TiXmlElement* parent, bool&         output);
bool readXmlElement(const std::string& name, const TiXmlElement* parent, std::vector<wxString>& output);


template <class T>
bool readXmlAttribute(const std::string& name, const TiXmlElement* node, T&            output);
bool readXmlAttribute(const std::string& name, const TiXmlElement* node, std::string&  output);
bool readXmlAttribute(const std::string& name, const TiXmlElement* node, wxString&     output);
bool readXmlAttribute(const std::string& name, const TiXmlElement* node, bool&         output);

template <class T>
void addXmlElement(const std::string& name, T                  value, TiXmlElement* parent);
void addXmlElement(const std::string& name, const std::string& value, TiXmlElement* parent);
void addXmlElement(const std::string& name, const wxString&    value, TiXmlElement* parent);
void addXmlElement(const std::string& name, const bool         value, TiXmlElement* parent);
void addXmlElement(const std::string& name, const std::vector<wxString>& value, TiXmlElement* parent);

template <class T>
void addXmlAttribute(const std::string& name, T                  value, TiXmlElement* node);
void addXmlAttribute(const std::string& name, const std::string& value, TiXmlElement* node);
void addXmlAttribute(const std::string& name, const wxString&    value, TiXmlElement* node);
void addXmlAttribute(const std::string& name, const bool         value, TiXmlElement* node);



class XmlParser
{
public:
    XmlParser(const TiXmlElement* rootElement) : root(rootElement) {}

    void logError(const std::string& nodeName);
    bool errorsOccured() const;
    const wxString getErrorMessageFormatted() const;

protected:
    //another level of indirection: if errors occur during xml parsing they are logged
    template <class T>
    void readXmlElementLogging(const std::string& name, const TiXmlElement* parent, T& output)
    {
        if (!readXmlElement(name, parent, output))
            logError(name);
    }

    template <class T>
    void readXmlAttributeLogging(const std::string& name, const TiXmlElement* node, T& output)
    {
        if (!readXmlAttribute(name, node, output))
            logError(name);
    }

    const TiXmlElement* const getRoot() const
    {
        return root;
    }

private:
    const TiXmlElement* const root;
    std::vector<wxString> failedNodes;
};
}


class TiXmlHandleConst //like TiXmlHandle but respecting "const TiXmlElement*"
{
public:
    TiXmlHandleConst(const TiXmlElement* element) : m_element(element) {}

    TiXmlHandleConst FirstChild(const char* name) const
    {
        return TiXmlHandleConst(m_element != NULL ? m_element->FirstChildElement(name) : NULL);
    }

    const TiXmlElement* ToElement() const
    {
        return m_element;
    }

private:
    const TiXmlElement* const m_element;
};










































//++++++++++++++ inline implementation +++++++++++++++++++++++++++++++++++++++
template <class T>
inline
bool xmlAccess::readXmlElement(const std::string& name, const TiXmlElement* parent, T& output)
{
    std::string temp;
    if (!readXmlElement(name, parent, temp))
        return false;

    globalFunctions::stringToNumber(temp, output);
    return true;
}


template <class T>
inline
void xmlAccess::addXmlElement(const std::string& name, T value, TiXmlElement* parent)
{
    addXmlElement(name, globalFunctions::numberToString(value), parent);
}


template <class T>
inline
bool xmlAccess::readXmlAttribute(const std::string& name, const TiXmlElement* node, T& output)
{
    std::string dummy;
    if (readXmlAttribute(name, node, dummy))
    {
        globalFunctions::stringToNumber(dummy, output);
        return true;
    }
    else
        return false;
}


template <class T>
inline
void xmlAccess::addXmlAttribute(const std::string& name, T value, TiXmlElement* node)
{
    addXmlAttribute(name, globalFunctions::numberToString(value), node);
}

#endif // XMLBASE_H_INCLUDED
