#ifndef THEMEDTABBEDPANEL_HPP
#define THEMEDTABBEDPANEL_HPP

#include <wx/wx.h>
#include <wx/simplebook.h>
#include <wx/scrolwin.h>
#include <wx/graphics.h>
#include <vector>

namespace Slic3r { namespace GUI {

// A themed container with custom tabs at the top and scrollable content pages.
// Replacement for wxNotebook which is hard to theme on Windows.
class ThemedTabbedPanel : public wxPanel {
public:
    ThemedTabbedPanel(wxWindow* parent, wxWindowID id = wxID_ANY, 
                      const wxPoint& pos = wxDefaultPosition, 
                      const wxSize& size = wxDefaultSize);

    // Add a new tab. Returns the scrollable window for that tab.
    // The specific page is pre-initialized with a vertical box sizer.
    wxScrolledWindow* AddTab(const wxString& label, const wxBitmapBundle& icon = wxBitmapBundle());

    // Selection Control
    void SetSelection(int n);
    int GetSelection() const { return m_selection; }
    size_t GetPageCount() const { return m_tabs.size(); }
    
    // Access header height for layout calculations if needed
    int GetHeaderHeight() const;

    // Remove all tabs and pages
    void Clear();

    wxSize DoGetBestSize() const override;

private:
    void OnPaint(wxPaintEvent& evt);
    void OnResize(wxSizeEvent& evt);
    void OnMouseLeftDown(wxMouseEvent& evt);
    void OnMouseMotion(wxMouseEvent& evt);
    void OnMouseLeave(wxMouseEvent& evt);

    struct TabEntry {
        wxString label;
        wxBitmapBundle icon;
        wxScrolledWindow* page;
        wxRect hitRect; 
    };

    std::vector<TabEntry> m_tabs;
    int m_selection = -1;
    int m_hoverIndex = -1;

    // Controls
    wxSimplebook* m_book;
};

}} // namespace Slic3r::GUI

#endif // THEMEDTABBEDPANEL_HPP
