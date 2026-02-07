#ifndef PLATE3D_HPP
#define PLATE3D_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include "ImGuiToolbar.hpp"
#include "Plater/PlaterObject.hpp"
#include "Scene3D.hpp"
#include "Settings.hpp"
#include "Model.hpp"
#include "Config.hpp"

namespace Slic3r { namespace GUI {

class Plate3D : public Scene3D {
public:
    Plate3D(wxWindow* parent, const wxSize& size, std::vector<PlaterObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config);
    
    /// Called to regenerate rendered volumes from the model 
    void update();
    
    /// Registered function to fire when objects are selected.
    std::function<void (const unsigned int obj_idx)> on_select_object {};
    
    /// Registered function to fire when an instance is moved.
    std::function<void ()> on_instances_moved {};

    /// Registered function to fire when the user right clicks.
    std::function<void(wxWindow* canvas, const wxPoint& pos)> on_right_click {};
    
    // Toolbar Callbacks (struct)
    PlaterActions m_actions;

    void selection_changed(){Refresh();}

 protected:
    virtual void render_imgui() override;
    // Render each volume as a different color and check what color is beneath
    // the mouse to determine the hovered volume
    void before_render();

    // Mouse events are needed to handle selecting and moving objects
    bool mouse_up(wxMouseEvent& e) override;
    bool mouse_move(wxMouseEvent& e) override;
    bool mouse_down(wxMouseEvent& e) override;

private:
    void color_volumes();
    Point pos, move_start;
    bool hover = false, mouse = false, moving = false;
    unsigned int hover_volume, hover_object, moving_volume;
    
    std::vector<PlaterObject>& objects; //< reference to parent vector
    std::shared_ptr<Slic3r::Model> model;
    std::shared_ptr<Slic3r::Config> config;
};

} } // Namespace Slic3r::GUI
#endif
