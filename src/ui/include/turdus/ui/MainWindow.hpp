#pragma once

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

namespace turdus::app {
class AppController;
}

namespace turdus::ui {

class TransportBar;
class PortsPanel;

// Application root window. Owns the menu bar and a vertical layout of:
//   [TransportBar]
//   [PortsPanel]
//   (Phase 7+ adds the piano roll below)
class MainWindow : public juce::DocumentWindow,
                   public juce::MenuBarModel {
public:
    explicit MainWindow(app::AppController& controller);
    ~MainWindow() override;

    void closeButtonPressed() override;

    // MenuBarModel
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int index, const juce::String& name) override;
    void menuItemSelected(int item_id, int index) override;

private:
    enum MenuItemIds {
        kFileNew = 1001,
        kFileOpen,
        kFileSave,
        kFileSaveAs,
        kFileQuit,
    };

    app::AppController& controller_;
    juce::File current_path_;
    std::unique_ptr<juce::FileChooser> active_chooser_;

    class Content;
    std::unique_ptr<Content> content_;

    void do_file_new();
    void do_file_open();
    void do_file_save();
    void do_file_save_as();
    void show_error(const juce::String& message);
};

}  // namespace turdus::ui
