#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include <turdus/app/AppController.hpp>
#include <turdus/midi/JuceMidiBackend.hpp>
#include <turdus/ui/MainWindow.hpp>

namespace turdus {

class TurdusApplication : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "Turdus"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String& /*commandLine*/) override {
        controller_ = std::make_unique<app::AppController>(midi::make_juce_backend());
        // Best-effort: pick the first available output port on startup so the user
        // can hit Play immediately if a port exists. They can change it from the
        // panel.
        if (auto ports = controller_->available_ports(); !ports.empty()) {
            controller_->set_active_port(ports.front());
        }
        main_window_ = std::make_unique<ui::MainWindow>(*controller_);
    }

    void shutdown() override {
        main_window_.reset();
        controller_.reset();
    }

    void systemRequestedQuit() override { quit(); }

    void anotherInstanceStarted(const juce::String& /*commandLine*/) override {}

private:
    std::unique_ptr<app::AppController> controller_;
    std::unique_ptr<ui::MainWindow> main_window_;
};

}  // namespace turdus

START_JUCE_APPLICATION(turdus::TurdusApplication)
