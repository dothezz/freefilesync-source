#include "structures.h"
#include "shared/fileHandling.h"
#include <wx/intl.h>
#include "shared/globalFunctions.h"


FreeFileSync::MainConfiguration::MainConfiguration() :
        compareVar(CMP_BY_TIME_SIZE),
        filterIsActive(false),        //do not filter by default
        includeFilter(wxT("*")),      //include all files/folders
        excludeFilter(wxEmptyString), //exclude nothing
        handleDeletion(FreeFileSync::recycleBinExists() ? MOVE_TO_RECYCLE_BIN : DELETE_PERMANENTLY) {} //enable if OS supports it; else user will have to activate first and then get an error message


wxString FreeFileSync::getVariantName(CompareVariant var)
{
    switch (var)
    {
    case CMP_BY_CONTENT:
        return _("File content");
    case CMP_BY_TIME_SIZE:
        return _("File size and date");
    }

    assert(false);
    return wxEmptyString;
}


using FreeFileSync::SyncConfiguration;


SyncConfiguration::Variant SyncConfiguration::getVariant()
{
    if (    exLeftSideOnly  == SYNC_DIR_CFG_RIGHT &&
            exRightSideOnly == SYNC_DIR_CFG_RIGHT &&
            leftNewer       == SYNC_DIR_CFG_RIGHT &&
            rightNewer      == SYNC_DIR_CFG_RIGHT &&
            different       == SYNC_DIR_CFG_RIGHT)
        return MIRROR;    //one way ->

    else if (exLeftSideOnly  == SYNC_DIR_CFG_RIGHT &&
             exRightSideOnly == SYNC_DIR_CFG_NONE  &&
             leftNewer       == SYNC_DIR_CFG_RIGHT &&
             rightNewer      == SYNC_DIR_CFG_NONE  &&
             different       == SYNC_DIR_CFG_NONE)
        return UPDATE;    //Update ->

    else if (exLeftSideOnly  == SYNC_DIR_CFG_RIGHT &&
             exRightSideOnly == SYNC_DIR_CFG_LEFT  &&
             leftNewer       == SYNC_DIR_CFG_RIGHT &&
             rightNewer      == SYNC_DIR_CFG_LEFT  &&
             different       == SYNC_DIR_CFG_NONE)
        return TWOWAY;    //two way <->
    else
        return CUSTOM;    //other
}


wxString SyncConfiguration::getVariantName()
{
    switch (getVariant())
    {
    case MIRROR:
        return _("Mirror ->>");
    case UPDATE:
        return _("Update ->");
    case TWOWAY:
        return _("Two way <->");
    case CUSTOM:
        return _("Custom");
    }

    return _("Error");
}


void SyncConfiguration::setVariant(const Variant var)
{
    switch (var)
    {
    case MIRROR:
        exLeftSideOnly  = SYNC_DIR_CFG_RIGHT;
        exRightSideOnly = SYNC_DIR_CFG_RIGHT;
        leftNewer       = SYNC_DIR_CFG_RIGHT;
        rightNewer      = SYNC_DIR_CFG_RIGHT;
        different       = SYNC_DIR_CFG_RIGHT;
        break;
    case UPDATE:
        exLeftSideOnly  = SYNC_DIR_CFG_RIGHT;
        exRightSideOnly = SYNC_DIR_CFG_NONE;
        leftNewer       = SYNC_DIR_CFG_RIGHT;
        rightNewer      = SYNC_DIR_CFG_NONE;
        different       = SYNC_DIR_CFG_NONE;
        break;
    case TWOWAY:
        exLeftSideOnly  = SYNC_DIR_CFG_RIGHT;
        exRightSideOnly = SYNC_DIR_CFG_LEFT;
        leftNewer       = SYNC_DIR_CFG_RIGHT;
        rightNewer      = SYNC_DIR_CFG_LEFT;
        different       = SYNC_DIR_CFG_NONE;
        break;
    case CUSTOM:
        assert(false);
        break;
    }
}
