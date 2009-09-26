#ifndef XMLFREEFILESYNC_H_INCLUDED
#define XMLFREEFILESYNC_H_INCLUDED

#include "xmlProcessing.h"


//reuse (some of) FreeFileSync's xml files

namespace RealtimeSync
{
    void readRealOrBatchConfig(const wxString& filename, xmlAccess::XmlRealConfig& config);  //throw (xmlAccess::XmlError);

    int getProgramLanguage();
}

#endif // XMLFREEFILESYNC_H_INCLUDED
