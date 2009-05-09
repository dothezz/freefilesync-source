#include "structures.h"
#include "library/fileHandling.h"

using namespace FreeFileSync;

MainConfiguration::MainConfiguration() :
        compareVar(CMP_BY_TIME_SIZE),
        filterIsActive(false),        //do not filter by default
        includeFilter(wxT("*")),      //include all files/folders
        excludeFilter(wxEmptyString), //exclude nothing
        useRecycleBin(FreeFileSync::recycleBinExists()) {} //enable if OS supports it; else user will have to activate first and then get an error message

