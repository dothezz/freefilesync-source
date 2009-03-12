/***************************************************************
 * Name:      mainDialog.h
 * Purpose:   Defines Application Frame
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2008-07-16
 **************************************************************/

#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include "guiGenerated.h"
#include "syncDialog.h"
#include "smallDialogs.h"
#include "../library/resources.h"
#include "../library/misc.h"
#include <wx/dnd.h>
#include <stack>
#include "../library/processXml.h"
#include <wx/event.h>


//IDs for context menu items
enum //context menu for left and right grids
{
    CONTEXT_FILTER_TEMP = 10,
    CONTEXT_EXCLUDE_EXT,
    CONTEXT_EXCLUDE_OBJ,
    CONTEXT_CLIPBOARD,
    CONTEXT_EXPLORER,
    CONTEXT_DELETE_FILES,
};

enum //context menu for middle grid
{
    CONTEXT_CHECK_ALL,
    CONTEXT_UNCHECK_ALL
};

enum //context menu for column settings
{
    CONTEXT_CUSTOMIZE_COLUMN_LEFT,
    CONTEXT_CUSTOMIZE_COLUMN_RIGHT
};

class CompareStatusHandler;
class FileDropEvent;
class FfsFileDropEvent;

class MainDialog : public MainDialogGenerated
{
    friend class CompareStatusHandler;
    friend class FileDropEvent;

public:
    MainDialog(wxFrame* frame, const wxString& cfgFileName, CustomLocale* language, xmlAccess::XmlGlobalSettings& settings);
    ~MainDialog();

private:
    void cleanUp();

    //configuration load/save
    bool readConfigurationFromXml(const wxString& filename, bool programStartup = false);
    bool writeConfigurationToXml(const wxString& filename);

    void readGlobalSettings();
    void writeGlobalSettings();

    void updateViewFilterButtons();
    void updateFilterButton(wxBitmapButton* filterButton, bool isActive);
    void updateCompareButtons();

    void addCfgFileToHistory(const wxString& filename);

    void addFolderPair(const wxString& leftDir, const wxString& rightDir);
    void removeFolderPair(bool removeAll = false);

    //main method for putting gridData on UI: maps data respecting current view settings
    void writeGrid(const FileCompareResult& gridData);
    void mapGridDataToUI(GridView& output, const FileCompareResult& fileCmpResult);
    void updateStatusInformation(const GridView& output);

    //context menu functions
    std::set<int> getSelectedRows(const wxGrid* grid);
    void filterRangeManually(const std::set<int>& rowsToFilterOnUiTable);
    void copySelectionToClipboard(const wxGrid* selectedGrid);
    void openWithFileManager(int rowNumber, const wxGrid* grid);
    void deleteFilesOnGrid(const std::set<int>& selectedRowsLeft, const std::set<int>& selectedRowsRight);

    //work to be done in idle time
    void OnIdleEvent(wxEvent& event);

    //delayed status information restore
    void pushStatusInformation(const wxString& text);
    void clearStatusBar();

    //events
    void onGridLeftAccess(  wxEvent& event);
    void onGridRightAccess( wxEvent& event);
    void onGridMiddleAccess(wxEvent& event);

    void onGridLeftButtonEvent(wxKeyEvent& event);
    void onGridRightButtonEvent(wxKeyEvent& event);
    void onGridMiddleButtonEvent(wxKeyEvent& event);
    void OnContextMenu(wxGridEvent& event);
    void OnContextMenuSelection(wxCommandEvent& event);
    void OnContextMenuMiddle(wxGridEvent& event);
    void OnContextMenuMiddleSelection(wxCommandEvent& event);
    void OnContextColumnLeft(wxGridEvent& event);
    void OnContextColumnRight(wxGridEvent& event);
    void OnContextColumnSelection(wxCommandEvent& event);

    void OnWriteDirManually(wxCommandEvent& event);
    void OnDirSelected(wxFileDirPickerEvent& event);

    //manual filtering of rows:
    void OnGridSelectCell(wxGridEvent& event);
    void OnGrid3LeftMouseUp(wxEvent& event);
    void OnGrid3LeftMouseDown(wxEvent& event);

    void OnLeftGridDoubleClick( wxGridEvent& event);
    void OnRightGridDoubleClick(wxGridEvent& event);
    void OnSortLeftGrid(        wxGridEvent& event);
    void OnSortRightGrid(       wxGridEvent& event);
    void OnSortMiddleGrid(      wxGridEvent& event);

    void OnLeftOnlyFiles(       wxCommandEvent& event);
    void OnLeftNewerFiles(      wxCommandEvent& event);
    void OnDifferentFiles(      wxCommandEvent& event);
    void OnRightNewerFiles(     wxCommandEvent& event);
    void OnRightOnlyFiles(      wxCommandEvent& event);
    void OnEqualFiles(          wxCommandEvent& event);

    void OnSaveConfig(          wxCommandEvent& event);
    void OnLoadConfig(          wxCommandEvent& event);
    void OnLoadFromHistory(     wxCommandEvent& event);
    void loadConfiguration(const wxString& filename);
    void OnChoiceKeyEvent(      wxKeyEvent& event );

    void OnFilesDropped(        FfsFileDropEvent& event);
    void onResizeMainWindow(    wxEvent& event);
    void OnAbortCompare(        wxCommandEvent& event);
    void OnFilterButton(        wxCommandEvent& event);
    void OnHideFilteredButton(  wxCommandEvent& event);
    void OnConfigureFilter(     wxHyperlinkEvent& event);
    void OnShowHelpDialog(      wxCommandEvent& event);
    void OnSwapDirs(            wxCommandEvent& event);
    void OnCompareByTimeSize(   wxCommandEvent& event);
    void OnCompareByContent(    wxCommandEvent& event);
    void OnCompare(             wxCommandEvent& event);
    void OnSync(                wxCommandEvent& event);
    void OnClose(               wxCloseEvent&   event);
    void OnQuit(                wxCommandEvent& event);

    void OnAddFolderPair(       wxCommandEvent& event);
    void OnRemoveFolderPair(    wxCommandEvent& event);

    //menu events
    void OnMenuSaveConfig(      wxCommandEvent& event);
    void OnMenuLoadConfig(      wxCommandEvent& event);
    void OnMenuGlobalSettings(  wxCommandEvent& event);
    void OnMenuExportFileList(  wxCommandEvent& event);
    void OnMenuBatchJob(        wxCommandEvent& event);
    void OnMenuAbout(           wxCommandEvent& event);
    void OnMenuQuit(            wxCommandEvent& event);
    void OnMenuLangChineseSimp( wxCommandEvent& event);
    void OnMenuLangDutch(       wxCommandEvent& event);
    void OnMenuLangEnglish(     wxCommandEvent& event);
    void OnMenuLangFrench(      wxCommandEvent& event);
    void OnMenuLangGerman(      wxCommandEvent& event);
    void OnMenuLangItalian(     wxCommandEvent& event);
    void OnMenuLangJapanese(    wxCommandEvent& event);
    void OnMenuLangPolish(      wxCommandEvent& event);
    void OnMenuLangPortuguese(  wxCommandEvent& event);

    void switchProgramLanguage(const int langID);
    void enableSynchronization(bool value);

//***********************************************
    //application variables are stored here:

    //global settings used by GUI and batch mode
    xmlAccess::XmlGlobalSettings& globalSettings;

    //technical representation of grid-data
    FileCompareResult currentGridData;

    //UI view of currentGridData
    GridView gridRefUI;

//-------------------------------------
    //functional configuration
    MainConfiguration cfg;

    //folder pairs:
    //m_directoryLeft, m_directoryRight
    std::vector<FolderPairGenerated*> additionalFolderPairs; //additional pairs to the standard pair

    //gui settings
    int widthNotMaximized;
    int heightNotMaximized;
    int posXNotMaximized;
    int posYNotMaximized;
    bool hideFilteredElements;
    bool ignoreErrors;
//-------------------------------------

    //convenience method to get all folder pairs (unformatted)
    std::vector<FolderPair> getFolderPairs();

    //UI View Filter settings
    bool leftOnlyFilesActive;
    bool leftNewerFilesActive;
    bool differentFilesActive;
    bool equalFilesActive;
    bool rightNewerFilesActive;
    bool rightOnlyFilesActive;

//***********************************************
    std::auto_ptr<wxMenu> contextMenu;

    CustomLocale* programLanguage;

    //status information
    wxLongLong lastStatusChange;
    std::stack<wxString> stackObjects;

    //compare status panel (hidden on start, shown when comparing)
    CompareStatus* compareStatus;

    //save the last used config filename history
    std::vector<wxString> cfgFileNames;

    //used when saving configuration
    wxString proposedConfigFileName;

    //variables for filtering of m_grid3
    bool filteringInitialized;
    bool filteringPending;

    //temporal variables used by exclude via context menu
    wxString exFilterCandidateExtension;
    struct FilterObject
    {
        wxString relativeName;
        FileDescrLine::ObjectType type;
    };
    std::vector<FilterObject> exFilterCandidateObj;

    bool synchronizationEnabled; //determines whether synchronization should be allowed

    CompareStatusHandler* cmpStatusHandlerTmp;  //used only by the abort button when comparing

    bool cleanedUp; //determines if destructor code was already executed

    //remember last sort executed (for determination of sort order)
    int lastSortColumn;
    const wxGrid* lastSortGrid;

    const wxGrid* leadGrid; //point to grid that is in focus
};

//######################################################################################

//define new event type
const wxEventType FFS_DROP_FILE_EVENT = wxNewEventType();
typedef void (wxEvtHandler::*FffsFileDropEventFunction)(FfsFileDropEvent&);
#define FfsFileDropEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(FffsFileDropEventFunction, &func)

class FfsFileDropEvent : public wxCommandEvent
{
public:
    FfsFileDropEvent(const wxString& nameDropped, const wxPanel* dropTarget) :
            wxCommandEvent(FFS_DROP_FILE_EVENT),
            m_nameDropped(nameDropped),
            m_dropTarget(dropTarget) {}

    virtual wxEvent* Clone() const
    {
        return new FfsFileDropEvent(m_nameDropped, m_dropTarget);
    }

    const wxString m_nameDropped;
    const wxPanel* m_dropTarget;
};


class MainWindowDropTarget : public wxFileDropTarget
{
public:
    MainWindowDropTarget(MainDialog* dlg, const wxPanel* obj) :
            mainDlg(dlg),
            dropTarget(obj) {}

    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);

private:
    MainDialog* mainDlg;
    const wxPanel* dropTarget;
};

//######################################################################################

//classes handling sync and compare error as well as status information

class CompareStatusHandler : public StatusHandler
{
public:
    CompareStatusHandler(MainDialog* dlg);
    ~CompareStatusHandler();

    void updateStatusText(const Zstring& text);
    void initNewProcess(int objectsTotal, double dataTotal, Process processID);
    void updateProcessedData(int objectsProcessed, double dataProcessed);
    void forceUiRefresh();

    ErrorHandler::Response reportError(const Zstring& text);
    void reportFatalError(const Zstring& errorMessage);
    void reportWarning(const Zstring& warningMessage, bool& dontShowAgain);

private:
    void abortThisProcess();

    MainDialog* mainDialog;
    bool ignoreErrors;
    Process currentProcess;
};


class SyncStatusHandler : public StatusHandler
{
public:
    SyncStatusHandler(wxWindow* dlg, bool ignoreAllErrors);
    ~SyncStatusHandler();

    void updateStatusText(const Zstring& text);
    void initNewProcess(int objectsTotal, double dataTotal, Process processID);
    void updateProcessedData(int objectsProcessed, double dataProcessed);
    void forceUiRefresh();

    ErrorHandler::Response reportError(const Zstring& text);
    void reportFatalError(const Zstring& errorMessage);
    void reportWarning(const Zstring& warningMessage, bool& dontShowAgain);

private:
    void abortThisProcess();

    SyncStatus* syncStatusFrame;
    bool ignoreErrors;
    wxArrayString unhandledErrors;   //list of non-resolved errors
};


#endif // MAINDIALOG_H
