// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef STD_BUTTON_ORDER_H_18347032147831732143214
#define STD_BUTTON_ORDER_H_18347032147831732143214

#include <wx/sizer.h>
#include <wx/button.h>

namespace zen
{
struct StdButtons
{
    StdButtons() : btnYes(nullptr), btnNo(nullptr), btnCancel(nullptr) {}
    StdButtons& setAffirmative (wxButton* btn) { btnYes    = btn; return *this; }
    StdButtons& setNegative    (wxButton* btn) { btnNo     = btn; return *this; }
    StdButtons& setCancel      (wxButton* btn) { btnCancel = btn; return *this; }

    wxButton* btnYes;
    wxButton* btnNo;
    wxButton* btnCancel;
};

void setStandardButtonOrder(wxBoxSizer& sizer, const StdButtons& buttons = StdButtons());
//sizer width will change! => call wxWindow::Fit and wxWindow::Layout











//--------------- impelementation -------------------------------------------
inline
void setStandardButtonOrder(wxBoxSizer& sizer, const StdButtons& buttons)
{
    assert(sizer.GetOrientation() == wxHORIZONTAL);

    StdButtons buttonsTmp = buttons;

    auto detach = [&](wxButton*& btn)
    {
        if (btn)
        {
            assert(btn->GetContainingSizer() == &sizer);
            if (btn->IsShown())
            {
                bool rv = sizer.Detach(btn);
                assert(rv);
                if (!rv)
                    btn = nullptr;
            }
            else
                btn = nullptr;
        }
    };

    detach(buttonsTmp.btnYes);
    detach(buttonsTmp.btnNo);
    detach(buttonsTmp.btnCancel);

#if defined ZEN_WIN
    //Windows User Experience Interaction Guidelines: http://msdn.microsoft.com/en-us/library/windows/desktop/aa511453.aspx#sizing
    const int spaceH    = 6;  //OK
    const int spaceRimH = 10; //OK
    const int spaceRimV = 8;  //compromise; consider additional top row from static line; exact values: top 8, bottom 9
#elif defined ZEN_LINUX
    //GNOME Human Interface Guidelines: https://developer.gnome.org/hig-book/3.2/hig-book.html#alert-spacing
    const int spaceH    = 6;  //OK
    const int spaceRimH = 12; //OK
    const int spaceRimV = 12; //OK
#elif defined ZEN_MAC
    //OS X Human Interface Guidelines: http://developer.apple.com/library/mac/#documentation/UserExperience/Conceptual/AppleHIGuidelines/Windows/Windows.html
    const int spaceH    = 14; //OK
    const int spaceRimH = 20; //OK
    const int spaceRimV = 11; //compromise; custom button height (of 30 pix) is considered, although the button is drawn smaller; otherwise 15 seems to have been optimal
#endif

    bool firstButtonSet = false;
    auto attach = [&](wxButton* btn)
    {
        if (btn)
        {
            if (firstButtonSet)
                sizer.Add(spaceH, 0);
            else
                firstButtonSet = true;
            sizer.Add(btn, 0, wxTOP | wxBOTTOM | wxALIGN_CENTER_VERTICAL, spaceRimV);
            assert(btn->GetSize().GetHeight() == 30); //see comment for OS X above
        }
    };

    //set border on left considering existing items
    if (sizer.GetChildren().GetCount() > 0) //for yet another retarded reason wxWidgets will have wxSizer::GetItem(0) cause an assert rather than just return nullptr as documented
        if (wxSizerItem* item = sizer.GetItem(static_cast<size_t>(0)))
        {
            assert(item->GetBorder() <= spaceRimV); //pragmatic check: other controls in the sizer should not have a larger border
            int flag = item->GetFlag();
            if (flag & wxLEFT)
            {
                flag &= ~wxLEFT;
                item->SetFlag(flag);
            }
            sizer.Insert(static_cast<size_t>(0), spaceRimH, 0);
        }

    sizer.Add(spaceRimH, 0);
#if defined ZEN_WIN
    attach(buttonsTmp.btnYes);
    attach(buttonsTmp.btnNo);
    attach(buttonsTmp.btnCancel);

#elif defined ZEN_LINUX
    attach(buttonsTmp.btnNo);
    attach(buttonsTmp.btnCancel);
    attach(buttonsTmp.btnYes);

#elif defined ZEN_MAC
    if (buttonsTmp.btnNo)
    {
        attach(buttonsTmp.btnNo);
        sizer.Add(42 - spaceH, 0); //OS X Human Interface Guidelines: "position it at least 24 pixels away from the “safe” buttons" -> however 42 is used in practice!
    }
    attach(buttonsTmp.btnCancel);
    attach(buttonsTmp.btnYes);
#endif
    sizer.Add(spaceRimH, 0);

    assert(buttonsTmp.btnCancel || buttonsTmp.btnYes); //OS X: there should be at least one button following the gap after the "dangerous" no-button
}
}

#endif //STD_BUTTON_ORDER_H_18347032147831732143214
