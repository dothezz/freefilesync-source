#include "toggleButton.h"

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
