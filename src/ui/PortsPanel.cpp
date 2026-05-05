#include <turdus/ui/PortsPanel.hpp>

#include <turdus/app/AppController.hpp>

namespace turdus::ui {

namespace {
constexpr int kNoneItemId = 1;
constexpr int kFirstPortId = 2;
}  // namespace

PortsPanel::PortsPanel(app::AppController& controller) : controller_(controller) {
    addAndMakeVisible(heading_);
    addAndMakeVisible(port_label_);
    addAndMakeVisible(port_combo_);
    addAndMakeVisible(refresh_button_);
    addAndMakeVisible(status_label_);

    heading_.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::bold)));

    refresh_button_.onClick = [this] { refresh_port_list(); };
    port_combo_.onChange = [this] { on_selection_changed(); };

    refresh_port_list();

    // Periodic refresh: ports may appear/disappear (USB device hotplug, virtual
    // ports created by other apps). Kept at 30s rather than 2s because on platforms
    // where the MIDI subsystem can't enumerate (WSL2 without ALSA seq), JUCE
    // re-prints an ALSA error to stderr on every call.
    startTimer(30'000);
}

PortsPanel::~PortsPanel() { stopTimer(); }

void PortsPanel::paint(juce::Graphics& g) {
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void PortsPanel::resized() {
    auto area = getLocalBounds().reduced(8);
    heading_.setBounds(area.removeFromTop(24));
    area.removeFromTop(4);

    auto row = area.removeFromTop(28);
    port_label_.setBounds(row.removeFromLeft(100));
    refresh_button_.setBounds(row.removeFromRight(80));
    row.removeFromRight(6);
    port_combo_.setBounds(row);

    area.removeFromTop(4);
    status_label_.setBounds(area.removeFromTop(20));
}

void PortsPanel::timerCallback() { refresh_port_list(); }

void PortsPanel::refresh_port_list() {
    auto ports = controller_.available_ports();
    const auto previous_selection = port_combo_.getText().toStdString();

    port_combo_.clear(juce::dontSendNotification);
    port_combo_.addItem("(none)", kNoneItemId);
    for (std::size_t i = 0; i < ports.size(); ++i) {
        port_combo_.addItem(juce::String(ports[i]),
                            static_cast<int>(kFirstPortId + i));
    }

    // Restore selection if the previously-active port still exists.
    const auto& active = controller_.active_port_name();
    int matched_id = kNoneItemId;
    if (!active.empty()) {
        for (std::size_t i = 0; i < ports.size(); ++i) {
            if (ports[i] == active) {
                matched_id = static_cast<int>(kFirstPortId + i);
                break;
            }
        }
    }
    port_combo_.setSelectedId(matched_id, juce::dontSendNotification);

    if (matched_id == kNoneItemId && !active.empty()) {
        status_label_.setText("Active port disappeared: " + juce::String(active),
                              juce::dontSendNotification);
    } else {
        status_label_.setText("", juce::dontSendNotification);
    }
}

void PortsPanel::on_selection_changed() {
    const auto id = port_combo_.getSelectedId();
    if (id == kNoneItemId) {
        controller_.set_active_port({});
        status_label_.setText("No active port", juce::dontSendNotification);
        return;
    }
    const auto name = port_combo_.getText().toStdString();
    if (controller_.set_active_port(name)) {
        status_label_.setText("Connected to " + juce::String(name),
                              juce::dontSendNotification);
    } else {
        status_label_.setText("Failed to open " + juce::String(name),
                              juce::dontSendNotification);
    }
}

}  // namespace turdus::ui
