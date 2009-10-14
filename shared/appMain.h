#ifndef APPMAIN_H_INCLUDED
#define APPMAIN_H_INCLUDED

class wxWindow;

namespace FreeFileSync
{
//just some wrapper around a global variable representing the (logical) main application window
class AppMainWindow
{
public:
    static void setMainWindow(wxWindow* window); //set main window and enable "exit on frame delete"
    static bool mainWindowWasSet();

private:
    static bool mainWndAct;
};
}


#endif // APPMAIN_H_INCLUDED
