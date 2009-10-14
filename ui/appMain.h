#ifndef APPMAIN_H_INCLUDED
#define APPMAIN_H_INCLUDED

#include <wx/app.h>

namespace FreeFileSync
{
class AppMainWindow
{
public:
    static void setMainWindow(wxWindow* window);
    static bool mainWindowActive();

private:
    static bool mainWndAct;
};
}


#endif // APPMAIN_H_INCLUDED
