#include <turdus/ui/MainWindow.hpp>

#include <utility>

#include <turdus/app/AppController.hpp>
#include <turdus/ui/PianoRollEditor.hpp>
#include <turdus/ui/PortsPanel.hpp>
#include <turdus/ui/TransportBar.hpp>

namespace turdus::ui {

// The root content holds the menu bar and the vertical stack of panels. JUCE's
// DocumentWindow expects a single setContentOwned() child, so we wrap everything
// in this Content component.
class MainWindow::Content : public juce::Component {
public:
    explicit Content(app::AppController& controller, juce::MenuBarModel& menu_model)
        : transport_(controller),
          ports_(controller),
          piano_roll_(controller),
          menu_(&menu_model) {
        addAndMakeVisible(menu_);
        addAndMakeVisible(transport_);
        addAndMakeVisible(ports_);
        addAndMakeVisible(piano_roll_);
        setSize(1100, 700);
    }

    void resized() override {
        auto area = getLocalBounds();
        menu_.setBounds(area.removeFromTop(juce::LookAndFeel::getDefaultLookAndFeel()
                                              .getDefaultMenuBarHeight()));
        transport_.setBounds(area.removeFromTop(56));
        ports_.setBounds(area.removeFromTop(80));
        piano_roll_.setBounds(area);
    }

private:
    TransportBar transport_;
    PortsPanel ports_;
    PianoRollEditor piano_roll_;
    juce::MenuBarComponent menu_;
};

MainWindow::MainWindow(app::AppController& controller)
    : juce::DocumentWindow("Turdus",
                           juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
                               juce::ResizableWindow::backgroundColourId),
                           juce::DocumentWindow::allButtons),
      controller_(controller) {
    content_ = std::make_unique<Content>(controller_, *this);
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setContentOwned(content_.release(), true);
    centreWithSize(1100, 700);
    setVisible(true);
}

MainWindow::~MainWindow() = default;

void MainWindow::closeButtonPressed() {
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

// ----- Menu ---------------------------------------------------------------------

juce::StringArray MainWindow::getMenuBarNames() { return {"File"}; }

juce::PopupMenu MainWindow::getMenuForIndex(int /*index*/, const juce::String& name) {
    juce::PopupMenu menu;
    if (name == "File") {
        menu.addItem(kFileNew, "New", true);
        menu.addItem(kFileOpen, "Open...", true);
        menu.addSeparator();
        menu.addItem(kFileSave, "Save", current_path_.existsAsFile());
        menu.addItem(kFileSaveAs, "Save As...", true);
        menu.addSeparator();
        menu.addItem(kFileQuit, "Quit", true);
    }
    return menu;
}

void MainWindow::menuItemSelected(int item_id, int /*index*/) {
    switch (item_id) {
        case kFileNew: do_file_new(); break;
        case kFileOpen: do_file_open(); break;
        case kFileSave: do_file_save(); break;
        case kFileSaveAs: do_file_save_as(); break;
        case kFileQuit:
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
            break;
        default: break;
    }
}

// ----- Actions ------------------------------------------------------------------

void MainWindow::do_file_new() {
    controller_.new_project();
    current_path_ = juce::File();
    repaint();
}

void MainWindow::do_file_open() {
    active_chooser_ = std::make_unique<juce::FileChooser>(
        "Open Turdus project", juce::File(), "*.turdus");
    active_chooser_->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) {
            const auto file = fc.getResult();
            if (!file.existsAsFile()) {
                return;
            }
            const auto path = file.getFullPathName().toStdString();
            if (controller_.open_project(path)) {
                current_path_ = file;
                repaint();
            } else {
                show_error("Failed to open project: " + file.getFullPathName());
            }
        });
}

void MainWindow::do_file_save() {
    if (!current_path_.existsAsFile()) {
        do_file_save_as();
        return;
    }
    if (!controller_.save_project(current_path_.getFullPathName().toStdString())) {
        show_error("Failed to save project to " + current_path_.getFullPathName());
    }
}

void MainWindow::do_file_save_as() {
    active_chooser_ = std::make_unique<juce::FileChooser>(
        "Save Turdus project", juce::File(), "*.turdus");
    active_chooser_->launchAsync(
        juce::FileBrowserComponent::saveMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::warnAboutOverwriting,
        [this](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file == juce::File()) {
                return;
            }
            if (!file.hasFileExtension(".turdus")) {
                file = file.withFileExtension(".turdus");
            }
            if (controller_.save_project(file.getFullPathName().toStdString())) {
                current_path_ = file;
                repaint();
            } else {
                show_error("Failed to save project to " + file.getFullPathName());
            }
        });
}

void MainWindow::show_error(const juce::String& message) {
    juce::AlertWindow::showAsync(
        juce::MessageBoxOptions()
            .withIconType(juce::MessageBoxIconType::WarningIcon)
            .withTitle("Turdus")
            .withMessage(message)
            .withButton("OK"),
        nullptr);
}

}  // namespace turdus::ui
