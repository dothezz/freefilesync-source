#ifndef XMLPROCESSING_H_INCLUDED
#define XMLPROCESSING_H_INCLUDED

#include <vector>
#include <wx/string.h>
#include "../shared/xmlBase.h"


namespace xmlAccess
{
    struct XmlRealConfig
    {
        XmlRealConfig() : delay(5) {}
        std::vector<wxString> directories;
        wxString commandline;
        unsigned int delay;
    };

    void readRealConfig(const wxString& filename, XmlRealConfig& config);           //throw (xmlAccess::XmlError);
    void writeRealConfig(const XmlRealConfig& outputCfg, const wxString& filename); //throw (xmlAccess::XmlError);
}

#endif // XMLPROCESSING_H_INCLUDED
