#ifndef SLIC3R_FIELD_HPP
#define SLIC3R_FIELD_HPP

#include <functional>
#include <string>
#include <limits>
#include <regex>
#include <tuple>

#include <boost/any.hpp>
#include "ConfigBase.hpp"
#include "Log.hpp"

#include "wx/spinctrl.h"
#include "wx/checkbox.h"
#include "wx/textctrl.h"
#include "wx/combobox.h"
#include "wx/choice.h"
#include "wx/arrstr.h"
#include "wx/stattext.h"
#include "wx/sizer.h"
#include "wx/ctrlsub.h"
#include <wx/colour.h>
#include <wx/clrpicker.h>
#include <wx/slider.h>
#include "../Widgets/ThemedControls.hpp"

namespace Slic3r { namespace GUI {

using namespace std::string_literals;

class UI_Field {
public:
    UI_Field(wxWindow* _parent, Slic3r::ConfigOptionDef _opt) : parent(_parent), opt(_opt) { };
    virtual ~UI_Field() = default; 

    bool disable_change_event {false};
    
    virtual void set_value(boost::any value) = 0;

    virtual void enable() { this->window->Enable(); }
    
    virtual void disable() { this->window->Disable(); }
    
    virtual void toggle(bool enable = true) { enable ? this->enable() : this->disable(); }

    virtual bool get_bool() { Slic3r::Log::warn(this->LogChannel(), "get_bool does not exist"s); return false; }
    virtual double get_double() { Slic3r::Log::warn(this->LogChannel(), "get_double does not exist"s); return 0.0; }
    virtual int get_int() { Slic3r::Log::warn(this->LogChannel(), "get_int does not exist"s); return 0; }
    virtual std::string get_string() { Slic3r::Log::warn(this->LogChannel(), "get_string does not exist"s); return ""; }

    virtual Slic3r::Pointf get_point() { Slic3r::Log::warn(this->LogChannel(), "get_point does not exist"s); return Slic3r::Pointf(); }
    virtual Slic3r::Pointf3 get_point3() { Slic3r::Log::warn(this->LogChannel(), "get_point3 does not exist"s); return Slic3r::Pointf3(); }

    virtual wxWindow* get_window() { return this->window; }

    virtual wxSizer* get_sizer() { return this->sizer; }

    std::function<void (const std::string&)> on_kill_focus {nullptr};

protected:
    wxWindow* parent {nullptr};
    wxWindow* window {nullptr};
    wxSizer* sizer {nullptr};

    const Slic3r::ConfigOptionDef opt;

    virtual std::string LogChannel() { return "UI_Field"s; }
    
    virtual void _on_change(std::string opt_id) = 0; 

    wxSize _default_size() { return wxSize((opt.width >= 0 ? opt.width : 60), (opt.height != -1 ? opt.height : -1)); }
    
public:
    virtual void set_dirty_status(bool dirty) {
        if (!this->window) return;
        if (dirty) {
             this->window->SetForegroundColour(wxColour(255, 128, 0)); // Orange
        } else {
             this->window->SetForegroundColour(wxNullColour); // Reset to default
        }
        this->window->Refresh();
    }
};


class UI_Window : public UI_Field { 
public:
    UI_Window(wxWindow* _parent, Slic3r::ConfigOptionDef _opt) : UI_Field(_parent, _opt) {};
    virtual ~UI_Window() = default; 
    virtual std::string LogChannel() override { return "UI_Window"s; }
};

class UI_Sizer : public UI_Field {
public:
    UI_Sizer(wxWindow* _parent, Slic3r::ConfigOptionDef _opt) : UI_Field(_parent, _opt) {};
    virtual ~UI_Sizer() = default; 
    virtual std::string LogChannel() override { return "UI_Sizer"s; }
};

class UI_Checkbox : public UI_Window {
public:
    UI_Checkbox(wxWindow* parent, Slic3r::ConfigOptionDef _opt, wxWindowID checkid = wxID_ANY) : UI_Window(parent, _opt) {
        _check = new ThemedCheckBox(parent, checkid, "");
        this->window = _check;

        if (this->opt.readonly) { this->_check->Disable(); }
        if (this->opt.default_value != nullptr) { this->_check->SetValue(this->opt.default_value->getBool()); }
        
        _check->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });
        _check->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { if (this->on_kill_focus != nullptr) {this->on_kill_focus("");} e.Skip(); });
    }

    ~UI_Checkbox() { }

    ThemedCheckBox* check() { return _check; }

    virtual bool get_bool() override { return _check->IsChecked();}

    virtual void set_value(boost::any value) override { this->_check->SetValue(boost::any_cast<bool>(value)); }

    std::function<void (const std::string&, bool value)> on_change {nullptr};

protected:
    virtual std::string LogChannel() override { return "UI_Checkbox"s; }

    void _on_change(std::string opt_id) override {
        if (!this->disable_change_event && this->window->IsEnabled() && this->on_change != nullptr) {
            this->on_change(opt_id, this->get_bool());
        }
    }
private:
    ThemedCheckBox* _check {nullptr};

};

class UI_SpinCtrl : public UI_Window {
public:
    UI_SpinCtrl(wxWindow* parent, Slic3r::ConfigOptionDef _opt, wxWindowID spinid = wxID_ANY) : UI_Window(parent, _opt) {

        int val = 0;
        try {
            if (opt.default_value != NULL) val = opt.default_value->getInt();
        } catch (...) {}

        _spin = new wxSpinCtrl(parent, spinid, "", wxDefaultPosition, _default_size(), 0,
            (opt.min > 0 ? (int)opt.min : 0), 
            (opt.max > 0 ? (int)opt.max : std::numeric_limits<int>::max()), 
            val);

        window = _spin;

        _spin->Bind(wxEVT_SPINCTRL, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });
        _spin->Bind(wxEVT_TEXT, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });
        _spin->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { 
            if (this->on_kill_focus != nullptr) this->on_kill_focus("");
            this->_on_change(""); 
            e.Skip(); 
        });
    }
    ~UI_SpinCtrl() { }
    int get_int() { return this->_spin->GetValue(); }
    void set_value(boost::any value) { this->_spin->SetValue(boost::any_cast<int>(value)); }

    wxSpinCtrl* spinctrl() { return _spin; }
    
    std::function<void (const std::string&, int value)> on_change {nullptr};

protected:
    virtual std::string LogChannel() { return "UI_SpinCtrl"s; }

    void _on_change(std::string opt_id) {
        if (!this->disable_change_event && this->window->IsEnabled() && this->on_change != nullptr) {
            this->on_change(opt_id, this->get_int());
        }
    }

private:
    wxSpinCtrl* _spin {nullptr};
};

class UI_TextCtrl : public UI_Window {
public:
    UI_TextCtrl(wxWindow* parent, Slic3r::ConfigOptionDef _opt, wxWindowID id = wxID_ANY) : UI_Window(parent, _opt) {
        int style {0};
        if (opt.multiline) {
            style |= wxHSCROLL;
            style |= wxTE_MULTILINE;
        } else {
            style |= wxTE_PROCESS_ENTER;
        }
        
        wxString default_val = "";
        if (opt.default_value != NULL) {
            try {
                default_val = wxString::FromUTF8(opt.default_value->serialize().c_str());
            } catch (...) { }
        }
        _text = new wxTextCtrl(parent, id, 
            default_val, 
            wxDefaultPosition, 
            _default_size(), 
            style);

        window = _text;

        if (!opt.multiline) {
            _text->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });
        }
        _text->Bind(wxEVT_TEXT, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });
        _text->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { 
            if (this->on_kill_focus != nullptr) this->on_kill_focus(""); 
            this->_on_change(""); 
            e.Skip(); 
        });
    }
    ~UI_TextCtrl() { }
    std::string get_string() { return this->_text->GetValue().ToStdString(); }
    void set_value(boost::any value) override { 
        try {
            this->_text->SetValue(boost::any_cast<std::string>(value)); 
        } catch (...) {
             try {
                 this->_text->SetValue(std::to_string(boost::any_cast<double>(value)));
             } catch (...) {}
        }
    }

    wxTextCtrl* textctrl() { return _text; }

    std::function<void (const std::string&, std::string value)> on_change {nullptr};

protected:
    virtual std::string LogChannel() { return "UI_TextCtrl"s; }

    void _on_change(std::string opt_id) {
        if (!this->disable_change_event && this->window->IsEnabled() && this->on_change != nullptr) {
            this->on_change(opt_id, this->get_string());
        }
    }

private:
    wxTextCtrl* _text {nullptr};
};

class UI_Choice : public UI_Window {
public:
    UI_Choice(wxWindow* parent, Slic3r::ConfigOptionDef _opt, wxWindowID id = wxID_ANY);
    ~UI_Choice() { }

    std::string get_string() override;

    void set_value(boost::any value) override;

    void set_dirty_status(bool dirty) override {
        if (this->_combo) {
             if (dirty) this->_combo->SetForegroundColour(wxColour(255, 128, 0));
             else this->_combo->SetForegroundColour(wxNullColour);
             this->_combo->Refresh();
        } else if (this->_choice) {
             if (dirty) this->_choice->SetForegroundColour(wxColour(255, 128, 0));
             else this->_choice->SetForegroundColour(wxNullColour);
             this->_choice->Refresh();
        }
        else {
            // base fallback
            UI_Field::set_dirty_status(dirty);
        }
    }

    std::function<void (const std::string&, std::string value)> on_change {nullptr};

    wxChoice* choice() { return this->_choice; }
    wxComboBox* combo() { return this->_combo; }
protected:
    virtual std::string LogChannel() override { return "UI_Choice"s; }

    void _on_change(std::string opt_id) override {
        if (!this->disable_change_event && this->window->IsEnabled() && this->on_change != nullptr) {
            this->on_change(opt_id, this->get_string());
        }
    }
private:
    wxComboBox* _combo {nullptr};
    wxChoice* _choice {nullptr};
};


class UI_NumChoice : public UI_Window {
public:
    UI_NumChoice(wxWindow* parent, Slic3r::ConfigOptionDef _opt, wxWindowID id = wxID_ANY);
    ~UI_NumChoice() { }

    std::string get_string() override;

    int get_int() override { return std::stoi(this->get_string()); }

    double get_double() override { return std::stod(this->get_string()); }


    wxComboBox* choice() { return this->_choice; }

    void set_value(boost::any value) override;

    void set_dirty_status(bool dirty) override {
        if (this->_choice) {
             if (dirty) this->_choice->SetForegroundColour(wxColour(255, 128, 0));
             else this->_choice->SetForegroundColour(wxNullColour);
             this->_choice->Refresh();
        } else {
             UI_Field::set_dirty_status(dirty);
        }
    }

    std::function<void (const std::string&, std::string value)> on_change {nullptr};

protected:
    virtual std::string LogChannel() override { return "UI_NumChoice"s; }

    void _set_value(int value, bool show_value = false);
    void _set_value(double value, bool show_value = false);
    void _set_value(std::string value);

    void _on_change(std::string opt_id) override {
        if (!this->disable_change_event && this->window->IsEnabled() && this->on_change != nullptr) {
            this->on_change(opt_id, this->get_string());
        }
    }
private:
    wxComboBox* _choice {nullptr};
    std::regex show_value_flag {"\bshow_value\b"};
};

class UI_Point : public UI_Sizer {
public:

    UI_Point(wxWindow* _parent, Slic3r::ConfigOptionDef _opt);
    ~UI_Point() { }
    std::string get_string() override;

    void set_value(boost::any value) override;

    Slic3r::Pointf get_point() override;
    Slic3r::Pointf3 get_point3() override;

    std::function<void (const std::string&, std::tuple<std::string, std::string> value)> on_change {nullptr};

    void enable() override { _ctrl_x->Enable(); _ctrl_y->Enable(); }
    void disable() override { _ctrl_x->Disable(); _ctrl_y->Disable(); }

    wxTextCtrl* ctrl_x() { return _ctrl_x;}
    wxTextCtrl* ctrl_y() { return _ctrl_y;}

    wxStaticText* lbl_x() { return _lbl_x;}
    wxStaticText* lbl_y() { return _lbl_y;}

protected:
    virtual std::string LogChannel() override { return "UI_Point"s; }

    void _on_change(std::string opt_id) override { 
        if (!this->disable_change_event && this->_ctrl_x->IsEnabled() && this->on_change != nullptr) {
            this->on_change(opt_id, std::make_tuple<std::string, std::string>(_ctrl_x->GetValue().ToStdString(), _ctrl_y->GetValue().ToStdString()));
        }
    }

private:
    wxSize field_size {40, 1};
    wxStaticText* _lbl_x {nullptr};
    wxStaticText* _lbl_y {nullptr};

    wxTextCtrl* _ctrl_x {nullptr};
    wxTextCtrl* _ctrl_y {nullptr};

    wxBoxSizer* _sizer {nullptr};

    void _set_value(Slic3r::Pointf value);
    void _set_value(Slic3r::Pointf3 value);
    void _set_value(std::string value);

};

class UI_Point3 : public UI_Sizer { 
public:
    UI_Point3(wxWindow* _parent, Slic3r::ConfigOptionDef _opt);
    ~UI_Point3() { }
    std::string get_string() override;

    void set_value(boost::any value) override;

    Slic3r::Pointf get_point() override;
    Slic3r::Pointf3 get_point3() override;

    void enable() override { _ctrl_x->Enable(); _ctrl_y->Enable(); _ctrl_z->Enable(); }
    void disable() override { _ctrl_x->Disable(); _ctrl_y->Disable(); _ctrl_z->Disable(); }

    std::function<void (const std::string&, std::tuple<std::string, std::string, std::string> value)> on_change {nullptr};

    wxTextCtrl* ctrl_x() { return _ctrl_x;}
    wxTextCtrl* ctrl_y() { return _ctrl_y;}
    wxTextCtrl* ctrl_z() { return _ctrl_z;}

    wxStaticText* lbl_x() { return _lbl_x;}
    wxStaticText* lbl_y() { return _lbl_y;}
    wxStaticText* lbl_z() { return _lbl_z;}

protected:
    virtual std::string LogChannel() override { return "UI_Point3"s; }

    void _on_change(std::string opt_id) override { 
        if (!this->disable_change_event && this->_ctrl_x->IsEnabled() && this->on_change != nullptr) {
            this->on_change(opt_id, std::make_tuple<std::string, std::string, std::string>(_ctrl_x->GetValue().ToStdString(), _ctrl_y->GetValue().ToStdString(),  _ctrl_z->GetValue().ToStdString()));
        }
    }

private:
    wxSize field_size {40, 1};
    wxStaticText* _lbl_x {nullptr};
    wxStaticText* _lbl_y {nullptr};
    wxStaticText* _lbl_z {nullptr};

    wxTextCtrl* _ctrl_x {nullptr};
    wxTextCtrl* _ctrl_y {nullptr};
    wxTextCtrl* _ctrl_z {nullptr};

    wxBoxSizer* _sizer {nullptr};

    void _set_value(Slic3r::Pointf value);
    void _set_value(Slic3r::Pointf3 value);
    void _set_value(std::string value);

};

class UI_Color : public UI_Window { 
public:
    UI_Color(wxWindow* parent, Slic3r::ConfigOptionDef _opt );  
    ~UI_Color() { }
    wxColourPickerCtrl* picker() { return this->_picker; }

    void set_value(boost::any value) override;
    std::string get_string() override; 
    std::function<void (const std::string&, const std::string&)> on_change {nullptr};
protected:
    virtual std::string LogChannel() override { return "UI_Color"s; }
    void _on_change(std::string opt_id) override {
        if (!this->disable_change_event && this->_picker->IsEnabled() && this->on_change != nullptr) {
            this->on_change(opt_id, _picker->GetColour().GetAsString(wxC2S_HTML_SYNTAX).ToStdString());
        }
    }
private:
    wxColour _string_to_color(const std::string& _color);
    wxColourPickerCtrl* _picker {nullptr};
};

class UI_Slider : public UI_Sizer { 
public:
    UI_Slider(wxWindow* parent, Slic3r::ConfigOptionDef _opt, size_t scale = 10);  

    ~UI_Slider();

    void set_value(boost::any value) override;
    std::string get_string() override;
    double get_double() override;
    int get_int() override;

    void enable() override;
    void disable() override;

    template <typename T> void set_range(T min, T max);

    void set_scale(size_t new_scale);
    
    wxSlider* slider() { return _slider;}
    wxTextCtrl* textctrl() { return _textctrl;}

    std::function<void (const std::string&, const double&)> on_change {nullptr};
protected:
    virtual std::string LogChannel() override { return "UI_Slider"s; }
    void set_dirty_status(bool dirty) override {
        if (this->_textctrl) {
             if (dirty) this->_textctrl->SetForegroundColour(wxColour(255, 128, 0));
             else this->_textctrl->SetForegroundColour(wxNullColour);
             this->_textctrl->Refresh();
        }
    }

private:
    void _on_change(std::string opt_id) override {
        if (!this->disable_change_event && this->_slider->IsEnabled() && this->on_change != nullptr) {
            this->on_change(opt_id, _slider->GetValue() / _scale);
        }
    }
    void _update_textctrl();
    wxTextCtrl* _textctrl {nullptr};
    wxSlider* _slider {nullptr};
    size_t _scale {10};
};

} }

#endif
