/* A backend-comparison harness for gmpi_ui.
 *
 * Four pages, cycled by clicking anywhere in the editor. The point is that the
 * SAME source runs through every backend (Direct2D, the software CpuGfx
 * renderer, JUCE, Cocoa, X11, Wayland), so the pages can be compared side by
 * side across hosts and platforms.
 *
 *   1  Shapes   geometry, stroking, the four brush types, alpha compositing
 *   2  Text     weights and styles, wrapping, colour emoji, alignment
 *   3  Colour   colour-space and banding checks, screen vs WIC bitmap
 *   4  Dialogs  every IDialogHost dialog, plus the host-driven context menu
 *
 * The Dialogs page is the one that needs real clicks, so it swallows them
 * rather than paging; page from the footer area or via the Page parameter.
 *
 * This supersedes gmpi_ui/examples/exampleJucePlugin, which could only ever run
 * under JUCE.
 */

#include "helpers/GmpiPluginEditor.h"

#include "demo_page_dialogs.h"
#include "demo_page_gradients.h"
#include "demo_page_shapes.h"
#include "demo_page_text.h"

using namespace gmpi;
using namespace gmpi::editor;
using namespace gmpi::drawing;

class DrawingDemoGui final : public PluginEditor
{
    enum Page { pageShapes, pageText, pageColour, pageDialogs, pageCount };

    // The page is a real parameter, so the host can automate it and a screenshot
    // harness can select a page without synthesising clicks.
    Pin<int32_t> pinPage;
    demo::GradientsPage gradients;
    demo::DialogsPage   dialogs;

    int page() const { return ((pinPage.value % pageCount) + pageCount) % pageCount; }

public:
    DrawingDemoGui()
    {
        pinPage.onUpdate = [this](PinBase*)
        {
            drawingHost->invalidateRect(nullptr);
        };
    }

    // The dialogs page needs the hosts, which only exist once setHost has run.
    ReturnCode setHost(gmpi::api::IUnknown* phost) override
    {
        const auto r = PluginEditor::setHost(phost);
        dialogs.setHosts(dialogHost.get(), drawingHost.get());
        return r;
    }

    // A FIXED-SIZE editor: the same answer whatever is offered, availableSize
    // ignored. That is the SDK's convention, not a shortcut — VST3EditorBase's
    // canResize() measures against 0x0 and 10000x10000 and calls a client
    // resizable only if the two answers DIFFER, so returning a constant is
    // precisely how a plugin declares it is not resizable.
    //
    // Both other readings are wrong, and each fails somewhere different:
    //   max(want, available) asked for 99999 x 99999 against the VST3 wrapper's
    //     unbounded probe, and DXGI refused to make that swap chain (a
    //     __debugbreak on attach in Ableton).
    //   min(want, available) shrank the editor to whatever a host happened to
    //     offer first — SynthEdit hands out a small default rect, so the whole
    //     demo collapsed to a ~100px box.
    //
    // The size matters: the colour page's ramps want one pixel per 8-bit code,
    // and below that they resample, making a missing level indistinguishable
    // from a dropped column — which is the very thing that page exists to show.
    ReturnCode measure(const Size* /*availableSize*/, Size* returnDesiredSize) override
    {
        returnDesiredSize->width  = (std::max)(demo::GradientsPage::preferredWidth,  demo::DialogsPage::preferredWidth);
        returnDesiredSize->height = (std::max)(demo::GradientsPage::preferredHeight, demo::DialogsPage::preferredHeight);
        return ReturnCode::Ok;
    }

    ReturnCode render(gmpi::drawing::api::IDeviceContext* drawingContext) override
    {
        Graphics g(drawingContext);
        ClipDrawingToBounds _(g, bounds);

        const Size size{ bounds.right - bounds.left, bounds.bottom - bounds.top };

        switch (page())
        {
        case pageShapes:  demo::drawShapesPage(g, size); break;
        case pageText:    demo::drawTextPage(g, size);   break;
        case pageColour:  gradients.draw(g, size);       break;
        case pageDialogs: dialogs.draw(g, size);         break;
        }

        drawFooter(g, size);
        return ReturnCode::Ok;
    }

    // Click to advance to the next page — except on the dialogs page, where a
    // click that lands on a button opens that dialog instead. The footer strip
    // always pages, so there is a way out without touching the Page parameter.
    ReturnCode onPointerUp(Point point, int32_t flags) override
    {
        const bool inFooter = point.y >= bounds.bottom - 18.0f;

        if (page() == pageDialogs && !inFooter && dialogs.onClick(point))
            return ReturnCode::Handled;

        pinPage = (page() + 1) % pageCount;
        drawingHost->invalidateRect(nullptr);
        return ReturnCode::Handled;
    }

    ReturnCode onPointerDown(Point point, int32_t flags) override
    {
        return ReturnCode::Handled; // claim the gesture so onPointerUp arrives
    }

    // Host-driven context menu — the opposite direction to the dialogs page's
    // buttons: here the host asks US to fill its menu.
    ReturnCode populateContextMenu(Point point, gmpi::api::IUnknown* sink) override
    {
        return dialogs.populateContextMenu(point, sink);
    }

private:
    void drawFooter(Graphics& g, Size size)
    {
        static const char* const names[] = { "1/4  Shapes", "2/4  Text", "3/4  Colour", "4/4  Dialogs" };

        auto font  = g.getFactory().createTextFormat(11.0f);
        auto brush = g.createSolidColorBrush(Color{ 1.0f, 1.0f, 1.0f, 0.45f });

        font.setTextAlignment(TextAlignment::Trailing);
        g.drawTextU(names[page()], font,
                    Rect{ 0.0f, size.height - 16.0f, size.width - 6.0f, size.height }, brush);

        font.setTextAlignment(TextAlignment::Leading);
        g.drawTextU("click to change page", font,
                    Rect{ 6.0f, size.height - 16.0f, size.width, size.height }, brush);
    }
};

namespace
{
auto r = Register<DrawingDemoGui>::withId("GMPI: DrawingDemo");
}
