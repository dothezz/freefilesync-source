// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) 2008-2011 ZenJu (zhnmju123 AT gmx.de)                    *
// **************************************************************************

#include "toggle_button.h"

void ToggleButton::init(const wxBitmap& activeBmp,
                        const wxString& activeTooltip,
                        const wxBitmap& inactiveBmp,
                        const wxString& inactiveTooltip)
{
    m_activeBmp       = activeBmp;
    m_activeTooltip   = activeTooltip;
    m_inactiveBmp     = inactiveBmp;
    m_inactiveTooltip = inactiveTooltip;

    //load resources
    setActive(active);
}


bool ToggleButton::isActive() const
{
    return active;
}


void ToggleButton::toggle()
{
    setActive(!active);
}


void ToggleButton::setActive(bool value)
{
    active = value;

    if (active)
    {
        SetBitmapLabel(m_activeBmp);
        SetToolTip(m_activeTooltip);
    }
    else
    {
        SetBitmapLabel(m_inactiveBmp);
        SetToolTip(m_inactiveTooltip);
    }
}
