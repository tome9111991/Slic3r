#ifndef WIDGET_GALLERY_HPP
#define WIDGET_GALLERY_HPP

#include <wx/wx.h>
#include <wx/dialog.h>

namespace Slic3r { namespace GUI {

class WidgetGallery : public wxDialog {
public:
    WidgetGallery(wxWindow* parent);
    
private:
    void InitGui();
    void OnClose(wxCommandEvent& evt);
};

}} // namespace Slic3r::GUI

#endif // WIDGET_GALLERY_HPP
