// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************
//
#ifndef XMLBASE_H_INCLUDED
#define XMLBASE_H_INCLUDED

#include <string>
#include <vector>
#include "tinyxml/tinyxml.h"
#include "string_tools.h"
#include <wx/string.h>
#include "xml_error.h"


namespace xmlAccess
{
/* Init XML document:

    TiXmlDocument doc;
    TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "UTF-8", "");
    doc.LinkEndChild(decl);  //ownership passed

    TiXmlElement* root = new TiXmlElement("HeaderName");
    doc.LinkEndChild(root);
*/
void loadXmlDocument(const wxString& fileName, TiXmlDocument& document);       //throw (XmlError)
void saveXmlDocument(const wxString& fileName, const TiXmlDocument& document); //throw (XmlError)


#ifndef TIXML_USE_STL
#error we need this macro
#endif

//------------------------------------------------------------------------------------------

//small helper functions
template <class T>
bool readXmlElement(const std::string& name, const TiXmlElement* parent, T&            output);
bool readXmlElement(const std::string& name, const TiXmlElement* parent, std::string&  output);
bool readXmlElement(const std::string& name, const TiXmlElement* parent, wxString&     output);
bool readXmlElement(const std::string& name, const TiXmlElement* parent, bool&         output);
bool readXmlElement(const std::string& name, const TiXmlElement* parent, std::vector<wxString>& output);

template <class T>
void addXmlElement(const std::string& name, T                  value, TiXmlElement* parent);
void addXmlElement(const std::string& name, const std::string& value, TiXmlElement* parent);
void addXmlElement(const std::string& name, const wxString&    value, TiXmlElement* parent);
void addXmlElement(const std::string& name, const bool         value, TiXmlElement* parent);
void addXmlElement(const std::string& name, const std::vector<wxString>& value, TiXmlElement* parent);

template <class T>
bool readXmlAttribute(const std::string& name, const TiXmlElement* node, T&            output);
bool readXmlAttribute(const std::string& name, const TiXmlElement* node, std::string&  output);
bool readXmlAttribute(const std::string& name, const TiXmlElement* node, wxString&     output);
bool readXmlAttribute(const std::string& name, const TiXmlElement* node, bool&         output);

template <class T>
void addXmlAttribute(const std::string& name, T                  value, TiXmlElement* node);
void addXmlAttribute(const std::string& name, const std::string& value, TiXmlElement* node);
void addXmlAttribute(const std::string& name, const wxString&    value, TiXmlElement* node);
void addXmlAttribute(const std::string& name, const bool         value, TiXmlElement* node);



class XmlErrorLogger
{
public:
    virtual ~XmlErrorLogger() {}

    void logError(const std::string& nodeName);
    bool errorsOccurred() const { return !failedNodes.empty(); }
    const wxString getErrorMessageFormatted() const;

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

private:
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

    const TiXmlElement* ToElement() const { return m_element; }

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

    output = zen::toNumber<T>(temp);
    return true;
}


template <class T>
inline
void xmlAccess::addXmlElement(const std::string& name, T value, TiXmlElement* parent)
{
    addXmlElement(name, zen::toString<std::string>(value), parent);
}


template <class T>
inline
bool xmlAccess::readXmlAttribute(const std::string& name, const TiXmlElement* node, T& output)
{
    std::string dummy;
    if (readXmlAttribute(name, node, dummy))
    {
        output = zen::toNumber<T>(dummy);
        return true;
    }
    else
        return false;
}


template <class T>
inline
void xmlAccess::addXmlAttribute(const std::string& name, T value, TiXmlElement* node)
{
    addXmlAttribute(name, zen::toString<std::string>(value), node);
}

#endif // XMLBASE_H_INCLUDED
