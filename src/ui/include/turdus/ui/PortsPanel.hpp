#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace turdus::app {
class AppController;
}

namespace turdus::ui {

// Lists available MIDI output ports and lets the user pick which one the engine
// drives. Phase 6 v0: single active output port. Per-track / multi-port routing
// arrives with the song view (Phase 9).
class PortsPanel : public juce::Component, private juce::Timer {
public:
    explicit PortsPanel(app::AppController& controller);
    ~PortsPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void refresh_port_list();
    void on_selection_changed();

    app::AppController& controller_;

    juce::Label heading_{"ports_heading", "MIDI Output"};
    juce::Label port_label_{"port_lbl", "Active port"};
    juce::ComboBox port_combo_;
    juce::TextButton refresh_button_{"Refresh"};
    juce::Label status_label_{"status_lbl", ""};
};

}  // namespace turdus::ui
