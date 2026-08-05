#pragma once

// Dialogs and other native UI, driven through IDialogHost.
//
// Every dialog the host can create gets a button; clicking one opens it and the
// result is written back onto the page. That makes the page useful for the thing
// that is otherwise painful to check — that a dialog actually appears, is modal
// in the right way, is positioned sensibly, and returns what it claims to on
// each backend (Win32, Cocoa, X11, Wayland all implement IDialogHost separately).
//
// Right-clicking anywhere also populates a context menu, which is a DIFFERENT
// path: the host asks the plugin to fill an IContextItemSink, rather than the
// plugin asking the host for a menu. Both are exercised here.
//
// LIFETIME is the subtle part of this API and the main thing this page
// demonstrates. showAsync() returns immediately; the dialog calls back later, on
// the UI thread. The callback objects are refcounted (GMPI_REFCOUNT in
// helpers/NativeUi.h), so they must outlive the call — hence they are members
// here, not locals. A callback built on the stack is a use-after-free that will
// usually appear to work, because the dialog often completes before the frame
// is reused.

#include <cstdint>
#include <string>

#include "GmpiUiDrawing.h"
#include "helpers/NativeUi.h"

namespace demo
{

class DialogsPage
{
public:
    enum class Action
    {
        PopupMenu,
        MessageBoxOk,
        MessageBoxYesNoCancel,
        ColorPicker,
        TextEdit,
        FileOpen,
        FileSave,
        FolderPick,
        KeyListener,
    };

private:
    struct Button
    {
        Action      action;
        const char* label;
        const char* note;
    };

    static constexpr float kBtnW   = 168.0f;
    static constexpr float kBtnH   = 30.0f;
    static constexpr float kGap    = 8.0f;
    static constexpr float kMargin = 12.0f;
    static constexpr int   kCols   = 3;

    static const Button* buttons(size_t& count)
    {
        static const Button b[] = {
            { Action::PopupMenu,             "Popup menu",       "submenu, tick, grey, separator" },
            { Action::MessageBoxOk,          "Message box: OK",  "stock dialog, one button" },
            { Action::MessageBoxYesNoCancel, "Message: Y/N/C",   "reports which button" },
            { Action::ColorPicker,           "Colour picker",    "seeded with the swatch" },
            { Action::TextEdit,              "Text edit",        "in-place editor" },
            { Action::FileOpen,              "File open",        "with extension filter" },
            { Action::FileSave,              "File save",        "initial name + folder" },
            { Action::FolderPick,            "Folder picker",    "FileDialogType::Folder" },
            { Action::KeyListener,           "Key listener",     "raw keys until focus lost" },
        };
        count = sizeof(b) / sizeof(b[0]);
        return b;
    }

    // Hosts. Set once by the editor before the page is used.
    gmpi::api::IDialogHost*  dialogHost{};
    gmpi::api::IDrawingHost* drawingHost{};

    // Callbacks MUST outlive showAsync() — see the note at the top of the file.
    gmpi::shared_ptr<gmpi::api::IUnknown> pendingDialog;
    gmpi::shared_ptr<gmpi::api::IUnknown> pendingCallback;

    // ...and TextEditCallback is the exception that proves the rule: it is
    // declared GMPI_REFCOUNT_NO_DELETE while every other callback helper in
    // helpers/NativeUi.h is GMPI_REFCOUNT. Heap-allocating it therefore leaks —
    // release() never frees it — so it has to be owned outright, like this.
    gmpi::sdk::TextEditCallback textEditCallback;

    std::string           lastResult{ "(nothing yet — click a button)" };
    std::string           lastAction;
    gmpi::drawing::Color  swatch{ 0.2f, 0.55f, 0.85f, 1.0f };
    std::string           editText{ "edit me" };

    void report(const char* action, std::string result)
    {
        lastAction = action;
        lastResult = std::move(result);
        if (drawingHost)
            drawingHost->invalidateRect(nullptr);
    }

    static gmpi::drawing::Rect buttonRect(int index)
    {
        const int   col = index % kCols;
        const int   row = index / kCols;
        const float x   = kMargin + col * (kBtnW + kGap);
        const float y   = 54.0f + row * (kBtnH + kGap);
        return { x, y, x + kBtnW, y + kBtnH };
    }

public:
    static constexpr float preferredWidth  = kMargin * 2 + kCols * kBtnW + (kCols - 1) * kGap;
    static constexpr float preferredHeight = 54.0f + 3 * (kBtnH + kGap) + 96.0f;

    void setHosts(gmpi::api::IDialogHost* dialogs, gmpi::api::IDrawingHost* drawing)
    {
        dialogHost  = dialogs;
        drawingHost = drawing;
    }

    // Returns true if a button was hit, so the editor knows not to treat the
    // click as "next page".
    bool onClick(gmpi::drawing::Point p)
    {
        size_t count{};
        const auto* b = buttons(count);
        for (int i = 0; i < static_cast<int>(count); ++i)
        {
            const auto r = buttonRect(i);
            if (p.x >= r.left && p.x < r.right && p.y >= r.top && p.y < r.bottom)
            {
                run(b[i].action, r);
                return true;
            }
        }
        return false;
    }

    // The other direction: the HOST asks us to fill its context menu.
    gmpi::ReturnCode populateContextMenu(gmpi::drawing::Point, gmpi::api::IUnknown* sink)
    {
        gmpi::shared_ptr<gmpi::api::IUnknown> unknown;
        unknown = sink;
        auto items = unknown.as<gmpi::api::IContextItemSink>();
        if (!items)
            return gmpi::ReturnCode::Unhandled;

        using F = gmpi::api::PopupMenuFlags;
        items->addItem("Context item one", 1, 0, nullptr);
        items->addItem("Ticked", 2, static_cast<int32_t>(F::Ticked), nullptr);
        items->addItem("Disabled", 3, static_cast<int32_t>(F::Grayed), nullptr);
        items->addItem("", 0, static_cast<int32_t>(F::Separator), nullptr);
        items->addItem("Reset result", 4, 0, nullptr);
        return gmpi::ReturnCode::Ok;
    }

    void draw(gmpi::drawing::Graphics& g, gmpi::drawing::Size size);

private:
    void run(Action action, gmpi::drawing::Rect anchor);
};

// --- implementation -------------------------------------------------------

inline void DialogsPage::draw(gmpi::drawing::Graphics& g, gmpi::drawing::Size size)
{
    using namespace gmpi::drawing;

    g.clear(colorFromHex(0x24282Cu));

    auto title    = g.getFactory().createTextFormat(15.0f);
    auto font     = g.getFactory().createTextFormat(12.0f);
    auto small    = g.getFactory().createTextFormat(10.0f);
    auto text     = g.createSolidColorBrush(Colors::WhiteSmoke);
    auto dim      = g.createSolidColorBrush(Colors::DarkGray);
    auto face     = g.createSolidColorBrush(colorFromHex(0x3C4249u));
    auto edge     = g.createSolidColorBrush(colorFromHex(0x5A626Bu));

    g.drawTextU("Dialogs — click a button; right-click anywhere for a context menu",
                title, Rect{ kMargin, kMargin, size.width - kMargin, kMargin + 20.0f }, text);

    if (!dialogHost)
    {
        auto warn = g.createSolidColorBrush(Colors::Orange);
        g.drawTextU("No IDialogHost — this host does not support dialogs.",
                    font, Rect{ kMargin, 34.0f, size.width - kMargin, 52.0f }, warn);
        return;
    }

    size_t count{};
    const auto* b = buttons(count);
    for (int i = 0; i < static_cast<int>(count); ++i)
    {
        const auto r = buttonRect(i);
        g.fillRectangle(r, face);
        g.drawRectangle(r, edge, 1.0f);
        g.drawTextU(b[i].label, font,
                    Rect{ r.left + 8.0f, r.top + 4.0f, r.right - 4.0f, r.top + 20.0f }, text);
        g.drawTextU(b[i].note, small,
                    Rect{ r.left + 8.0f, r.top + 17.0f, r.right - 4.0f, r.bottom }, dim);
    }

    // Result panel.
    const float y = 54.0f + 3 * (kBtnH + kGap) + 6.0f;
    g.drawTextU(lastAction.empty() ? "Result" : ("Result — " + lastAction).c_str(),
                small, Rect{ kMargin, y, size.width - kMargin, y + 14.0f }, dim);
    g.drawTextU(lastResult.c_str(), font,
                Rect{ kMargin, y + 15.0f, size.width - kMargin, y + 50.0f }, text);

    // Colour swatch, so the picker's result is visible as colour, not just text.
    const Rect sw{ size.width - kMargin - 48.0f, y, size.width - kMargin, y + 34.0f };
    auto swatchBrush = g.createSolidColorBrush(swatch);
    g.fillRectangle(sw, swatchBrush);
    g.drawRectangle(sw, edge, 1.0f);
}

inline void DialogsPage::run(Action action, gmpi::drawing::Rect anchor)
{
    using namespace gmpi::api; // the interfaces
    using namespace gmpi::sdk; // the std::function callback wrappers

    if (!dialogHost)
        return;

    // Drop any previous dialog/callback pair first: releasing here (rather than
    // on completion) keeps exactly one alive and makes the ownership obvious.
    pendingDialog  = {};
    pendingCallback = {};

    gmpi::shared_ptr<IUnknown> unknown;

    switch (action)
    {
    case Action::PopupMenu:
    {
        if (dialogHost->createPopupMenu(&anchor, unknown.put()) != gmpi::ReturnCode::Ok)
            return report("popup menu", "createPopupMenu failed");

        auto menu = unknown.as<IPopupMenu>();
        if (!menu)
            return report("popup menu", "no IPopupMenu interface");

        // One callback shared by every item; the host reports which id won.
        gmpi::shared_ptr<IUnknown> cb;
        cb.attach(new PopupMenuCallback(
            [this](int32_t id) { report("popup menu", "selected id " + std::to_string(id)); },
            [this]()           { report("popup menu", "cancelled"); }));

        using F = PopupMenuFlags;
        menu->addItem("Plain item",  10, 0, cb.get());
        menu->addItem("Ticked item", 11, static_cast<int32_t>(F::Ticked), cb.get());
        menu->addItem("Greyed item", 12, static_cast<int32_t>(F::Grayed), cb.get());
        menu->addItem("",             0, static_cast<int32_t>(F::Separator), cb.get());
        menu->addItem("Submenu",      0, static_cast<int32_t>(F::SubMenuBegin), cb.get());
        menu->addItem("Nested one",  20, 0, cb.get());
        menu->addItem("Nested two",  21, 0, cb.get());
        menu->addItem("",             0, static_cast<int32_t>(F::SubMenuEnd), cb.get());

        pendingCallback = cb;
        pendingDialog   = unknown;
        menu->showAsync();
        break;
    }

    case Action::MessageBoxOk:
    case Action::MessageBoxYesNoCancel:
    {
        const bool multi = action == Action::MessageBoxYesNoCancel;
        const auto type  = multi ? StockDialogType::YesNoCancel : StockDialogType::Ok;

        if (dialogHost->createStockDialog(
                static_cast<int32_t>(type),
                multi ? "Discard changes?" : "Drawing Demo",
                multi ? "Three buttons, so the result actually carries information."
                      : "A stock message box with a single OK button.",
                unknown.put()) != gmpi::ReturnCode::Ok)
            return report("message box", "createStockDialog failed");

        auto dlg = unknown.as<IStockDialog>();
        if (!dlg)
            return report("message box", "no IStockDialog interface");

        gmpi::shared_ptr<IUnknown> cb;
        cb.attach(new StockDialogCallback([this](StockDialogButton button)
        {
            const char* name = "?";
            switch (button)
            {
            case StockDialogButton::Ok:     name = "Ok";     break;
            case StockDialogButton::Cancel: name = "Cancel"; break;
            case StockDialogButton::Yes:    name = "Yes";    break;
            case StockDialogButton::No:     name = "No";     break;
            }
            report("message box", std::string("pressed ") + name);
        }));

        pendingCallback = cb;
        pendingDialog   = unknown;
        dlg->showAsync(cb.get());
        break;
    }

    case Action::ColorPicker:
    {
        if (dialogHost->createColorDialog(swatch, unknown.put()) != gmpi::ReturnCode::Ok)
            return report("colour picker", "createColorDialog failed");

        auto dlg = unknown.as<IColorDialog>();
        if (!dlg)
            return report("colour picker", "no IColorDialog interface");

        gmpi::shared_ptr<IUnknown> cb;
        cb.attach(new ColorDialogCallback([this](gmpi::drawing::Color c)
        {
            swatch = c;
            char buf[96];
            snprintf(buf, sizeof buf, "r %.3f  g %.3f  b %.3f  a %.3f (linear)", c.r, c.g, c.b, c.a);
            report("colour picker", buf);
        }));

        pendingCallback = cb;
        pendingDialog   = unknown;
        dlg->showAsync(cb.get());
        break;
    }

    case Action::TextEdit:
    {
        if (dialogHost->createTextEdit(&anchor, unknown.put()) != gmpi::ReturnCode::Ok)
            return report("text edit", "createTextEdit failed");

        auto edit = unknown.as<ITextEdit>();
        if (!edit)
            return report("text edit", "no ITextEdit interface");

        edit->setText(editText.c_str());
        edit->setTextSize(12.0f);
        edit->setAlignment(static_cast<int32_t>(gmpi::drawing::TextAlignment::Leading));

        textEditCallback.onSuccess = [this](const std::string& t)
        {
            editText = t;
            report("text edit", "committed: " + editText);
        };
        textEditCallback.onCancel = [this]() { report("text edit", "cancelled"); };

        pendingDialog = unknown;
        edit->showAsync(&textEditCallback);
        break;
    }

    case Action::FileOpen:
    case Action::FileSave:
    case Action::FolderPick:
    {
        const auto type = action == Action::FileOpen ? FileDialogType::Open
                        : action == Action::FileSave ? FileDialogType::Save
                                                     : FileDialogType::Folder;
        const char* what = action == Action::FileOpen ? "file open"
                         : action == Action::FileSave ? "file save"
                                                      : "folder picker";

        if (dialogHost->createFileDialog(static_cast<int32_t>(type), unknown.put()) != gmpi::ReturnCode::Ok)
            return report(what, "createFileDialog failed");

        auto dlg = unknown.as<IFileDialog>();
        if (!dlg)
            return report(what, "no IFileDialog interface");

        if (type != FileDialogType::Folder)
        {
            dlg->addExtension("wav", "Wave audio");
            dlg->addExtension("png", "PNG image");
        }
        if (type == FileDialogType::Save)
            dlg->setInitialFilename("untitled.wav");

        gmpi::shared_ptr<IUnknown> cb;
        cb.attach(new FileDialogCallback(
            [this, what](const std::string& path) { report(what, path.empty() ? "(empty path)" : path); },
            [this, what]()                        { report(what, "cancelled"); }));

        pendingCallback = cb;
        pendingDialog   = unknown;
        dlg->showAsync(&anchor, cb.get());
        break;
    }

    case Action::KeyListener:
    {
        if (dialogHost->createKeyListener(&anchor, unknown.put()) != gmpi::ReturnCode::Ok)
            return report("key listener", "createKeyListener failed");

        auto listener = unknown.as<IKeyListener>();
        if (!listener)
            return report("key listener", "no IKeyListener interface");

        // A real callback, not nullptr. Passing null here read as harmless —
        // "I do not care about the keys" — and instead crashed the host, since
        // the Win32 listener dereferences it immediately.
        gmpi::shared_ptr<IUnknown> cb;
        auto* keys = new KeyListenerCallback(
            [this](int32_t key, int32_t flags)
            {
                char buf[96];
                snprintf(buf, sizeof buf, "key down 0x%02X (%c)  flags 0x%X",
                         key, (key >= 32 && key < 127) ? char(key) : '.', flags);
                report("key listener", buf);
            },
            {}, // key up: ignored, it would just overwrite the more useful line
            [this]() { report("key listener", "focus lost — listener closed"); });

        keys->onCopy  = [this]() { return editText; };
        keys->onPaste = [this](std::string_view t) { report("key listener", "pasted: " + std::string(t)); };
        cb.attach(keys);

        report("key listener", "listening — press keys, click away to end");
        pendingCallback = cb;
        pendingDialog   = unknown;
        listener->showAsync(cb.get());
        break;
    }
    }
}

} // namespace demo
