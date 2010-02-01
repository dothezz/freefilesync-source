#ifndef RESOURCES_H_INCLUDED
#define RESOURCES_H_INCLUDED

#include <wx/bitmap.h>
#include <wx/animate.h>
#include <wx/string.h>
#include <map>


class GlobalResources
{
public:
    static const GlobalResources& getInstance();

    const wxBitmap& getImageByName(const wxString& imageName) const;

    //global image resource objects
    wxAnimation* animationMoney;
    wxAnimation* animationSync;
    wxIcon* programIcon;

    void load() const; //loads bitmap resources on program startup: logical const!

private:
    GlobalResources();
    ~GlobalResources();

    //resource mapping
    mutable std::map<wxString, wxBitmap*> bitmapResource;
};

#endif // RESOURCES_H_INCLUDED
