// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2010 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef XMLBASE_H_INCLUDED
#define XMLBASE_H_INCLUDED

#include "tinyxml/tinyxml.h"
#include <string>
#include <vector>
#include <wx/string.h>


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

XmlType getXmlType(const wxString& filename);

bool loadXmlDocument(const wxString& fileName, const XmlType type, TiXmlDocument& document);
void getDefaultXmlDocument(const XmlType type, TiXmlDocument& document);
bool saveXmlDocument(const wxString& fileName, const TiXmlDocument& document);


//------------------------------------------------------------------------------------------

//small helper functions
bool readXmlElement(const std::string& name, const TiXmlElement* parent, std::string&  output);
bool readXmlElement(const std::string& name, const TiXmlElement* parent, wxString&     output);
bool readXmlElement(const std::string& name, const TiXmlElement* parent, int&          output);
bool readXmlElement(const std::string& name, const TiXmlElement* parent, unsigned int& output);
bool readXmlElement(const std::string& name, const TiXmlElement* parent, long&         output);
bool readXmlElement(const std::string& name, const TiXmlElement* parent, bool&         output);
bool readXmlElement(const std::string& name, const TiXmlElement* parent, std::vector<wxString>& output);

bool readXmlAttribute(const std::string& name, const TiXmlElement* node, std::string&  output);
bool readXmlAttribute(const std::string& name, const TiXmlElement* node, wxString&     output);
bool readXmlAttribute(const std::string& name, const TiXmlElement* node, int&          output);
bool readXmlAttribute(const std::string& name, const TiXmlElement* node, unsigned int& output);
bool readXmlAttribute(const std::string& name, const TiXmlElement* node, bool&         output);

void addXmlElement(const std::string& name, const std::string& value, TiXmlElement* parent);
void addXmlElement(const std::string& name, const wxString&    value, TiXmlElement* parent);
void addXmlElement(const std::string& name, const int          value, TiXmlElement* parent);
void addXmlElement(const std::string& name, const unsigned int value, TiXmlElement* parent);
void addXmlElement(const std::string& name, const long         value, TiXmlElement* parent);
void addXmlElement(const std::string& name, const bool         value, TiXmlElement* parent);
void addXmlElement(const std::string& name, const std::vector<wxString>& value, TiXmlElement* parent);

void addXmlAttribute(const std::string& name, const std::string& value, TiXmlElement* node);
void addXmlAttribute(const std::string& name, const wxString&    value, TiXmlElement* node);
void addXmlAttribute(const std::string& name, const int          value, TiXmlElement* node);
void addXmlAttribute(const std::string& name, const unsigned int value, TiXmlElement* node);
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

    const TiXmlElement* const root;
    std::vector<wxString> failedNodes;
};


class XmlError //Exception class
{
public:
    enum Severity
    {
        WARNING = 77,
        FATAL
    };

    XmlError(const wxString& message, Severity sev = FATAL) : errorMessage(message), m_severity(sev) {}

    const wxString& show() const
    {
        return errorMessage;
    }
    Severity getSeverity() const
    {
        return m_severity;
    }
private:
    const wxString errorMessage;
    const Severity m_severity;
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


#endif // XMLBASE_H_INCLUDED
