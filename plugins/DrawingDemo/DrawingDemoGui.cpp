/* A backend-comparison harness for gmpi_ui.
 *
 * Three pages of drawing, cycled by clicking anywhere in the editor. The point
 * is that the SAME source renders through every backend (Direct2D, the software
 * CpuGfx renderer, JUCE, Cocoa), so the pages can be compared side by side
 * across hosts and platforms.
 *
 *   1  Shapes  geometry, stroking, the four brush types, alpha compositing
 *   2  Text    weights and styles, wrapping, colour emoji, alignment
 *   3  Colour  colour-space and banding checks, screen vs WIC bitmap
 *
 * This supersedes gmpi_ui/examples/exampleJucePlugin, which could only ever run
 * under JUCE.
 */

#include "helpers/GmpiPluginEditor.h"

#include "demo_page_gradients.h"
#include "demo_page_shapes.h"
#include "demo_page_text.h"

using namespace gmpi;
using namespace gmpi::editor;
using namespace gmpi::drawing;

class DrawingDemoGui final : public PluginEditor
{
    enum Page { pageShapes, pageText, pageColour, pageCount };

    // The page is a real parameter, so the host can automate it and a screenshot
    // harness can select a page without synthesising clicks.
    Pin<int32_t> pinPage;
    demo::GradientsPage gradients;

    int page() const { return ((pinPage.value % pageCount) + pageCount) % pageCount; }

public:
    DrawingDemoGui()
    {
        pinPage.onUpdate = [this](PinBase*)
        {
            drawingHost->invalidateRect(nullptr);
        };
    }

    // Ask for a window big enough that the colour page's ramps get one pixel per
    // 8-bit code. Below that the ramps resample and banding becomes ambiguous:
    // you cannot tell a missing level from a dropped column.
    ReturnCode measure(const Size* availableSize, Size* returnDesiredSize) override
    {
        returnDesiredSize->width  = (std::max)(demo::GradientsPage::preferredWidth,  availableSize->width);
        returnDesiredSize->height = (std::max)(demo::GradientsPage::preferredHeight, availableSize->height);
        return ReturnCode::Ok;
    }

    ReturnCode render(gmpi::drawing::api::IDeviceContext* drawingContext) override
    {
        Graphics g(drawingContext);
        ClipDrawingToBounds _(g, bounds);

        const Size size{ bounds.right - bounds.left, bounds.bottom - bounds.top };

        switch (page())
        {
        case pageShapes: demo::drawShapesPage(g, size); break;
        case pageText:   demo::drawTextPage(g, size);   break;
        case pageColour: gradients.draw(g, size);       break;
        }

        drawFooter(g, size);
        return ReturnCode::Ok;
    }

    // Click anywhere to advance to the next page.
    ReturnCode onPointerUp(Point point, int32_t flags) override
    {
        pinPage = (page() + 1) % pageCount;
        drawingHost->invalidateRect(nullptr);
        return ReturnCode::Handled;
    }

    ReturnCode onPointerDown(Point point, int32_t flags) override
    {
        return ReturnCode::Handled; // claim the gesture so onPointerUp arrives
    }

private:
    void drawFooter(Graphics& g, Size size)
    {
        static const char* const names[] = { "1/3  Shapes", "2/3  Text", "3/3  Colour" };

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
