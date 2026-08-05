#pragma once

// Geometry, stroking and fills: line caps, dash styles, line joins, beziers,
// the four brush types (solid, linear gradient, radial gradient, bitmap) as both
// fills and strokes, and an alpha-compositing check.
//
// Ported from gmpi_ui/examples/exampleJucePlugin, so the same scene can be
// compared between the JUCE backend and the native ones.

#include <cstdint>

#include "GmpiUiDrawing.h"

namespace demo
{

namespace detail
{

struct lineStyle
{
    gmpi::drawing::CapStyle  capStyle;
    gmpi::drawing::Color     color;
    gmpi::drawing::DashStyle dashStyle;
};

struct triangleStyle
{
    gmpi::drawing::Color    color;
    gmpi::drawing::LineJoin lineJoin;
};

// A high-contrast tartan, used as a bitmap-brush source. At file scope because
// both the fill and the stroke example need it — in the JUCE original this was a
// lambda inside the fill block, and the stroke block referenced it out of scope.
inline uint32_t tartanPixel(gmpi::drawing::BitmapPixels& pixels, int x, int y)
{
    uint32_t col = pixels.rgBytesToPixel(0x42, 0x73, 0x9e); // blue

    const int index = (x & 1) ^ (y & 1) ? x : y;

    if ((index >> 3) % 2 == 0)
        col = pixels.rgBytesToPixel(0, 0, 0); // black
    else if ((index >> 1) % 4 == 2)
        col = pixels.rgBytesToPixel(0xff, 0xff, 0xff); // white

    return col;
}

inline gmpi::drawing::Bitmap makeTartan(gmpi::drawing::Graphics& g, uint32_t sz = 128)
{
    auto bitmap = g.getFactory().createImage(
        gmpi::drawing::SizeU{ sz, sz },
        static_cast<int32_t>(gmpi::drawing::BitmapRenderTargetFlags::SRGBPixels));
    {
        auto pixels = bitmap.lockPixels(gmpi::drawing::BitmapLockFlags::Write);
        if (pixels)
        {
            for (uint32_t py = 0; py < sz; ++py)
                for (uint32_t px = 0; px < sz; ++px)
                    pixels.setPixel(px, py, tartanPixel(pixels, int(px), int(py)));
        }
    }
    return bitmap;
}

} // namespace detail

inline void drawShapesPage(gmpi::drawing::Graphics& g, gmpi::drawing::Size size)
{
    using namespace gmpi::drawing;

    g.clear(colorFromHex(0x323E44u));

    const float margin = size.height / 24.0f;

    const detail::lineStyle lineStyles[] = {
        { CapStyle::Flat,   Colors::Salmon,        DashStyle::Solid },
        { CapStyle::Round,  Colors::DarkOrange,    DashStyle::Solid },
        { CapStyle::Square, Colors::Gold,          DashStyle::Solid },
        { CapStyle::Square, Colors::LightSeaGreen, DashStyle::Dash  },
        { CapStyle::Round,  Colors::DodgerBlue,    DashStyle::Dot   }
    };

    float y  = margin;
    float x1 = margin;
    const float x2 = 98.0f;

    auto brush1 = g.createSolidColorBrush(Colors::Green);

    for (const auto& style : lineStyles)
    {
        StrokeStyleProperties strokeStyleProperties{};
        strokeStyleProperties.lineCap   = style.capStyle;
        strokeStyleProperties.dashStyle = style.dashStyle;

        auto strokeStyle = g.getFactory().createStrokeStyle(strokeStyleProperties);
        brush1.setColor(style.color);

        g.drawLine({ x1, y }, { x2, y }, brush1, 6.0f, strokeStyle);
        y += margin;
    }

    // Line joins.
    const detail::triangleStyle linejoins[] = {
        { Colors::Firebrick,  LineJoin::Bevel },
        { Colors::Coral,      LineJoin::Miter },
        { Colors::Aquamarine, LineJoin::Round }
    };

    x1 = 120.0f;
    y  = 50.0f;
    float side = 30.0f;

    for (const auto& style : linejoins)
    {
        StrokeStyleProperties strokeStyleProperties{};
        strokeStyleProperties.lineJoin = style.lineJoin;

        auto strokeStyle = g.getFactory().createStrokeStyle(strokeStyleProperties);

        auto geometry = g.getFactory().createPathGeometry();
        auto sink = geometry.open();
        sink.beginFigure({ x1, y });
        sink.addLine({ x1 + side, y });
        sink.addLine({ x1 + side * 0.5f, y - side * 0.866f });
        sink.endFigure(FigureEnd::Closed);
        sink.close();

        brush1.setColor(style.color);
        g.drawGeometry(geometry, brush1, 6.0f, strokeStyle);

        x1 += side * 1.5f;
    }

    // Bezier curves.
    x1 += side;
    side = 70.0f;
    {
        brush1.setColor(Colors::GreenYellow);
        for (float dx = -side; dx < side; dx += 10.0f)
        {
            auto geometry = g.getFactory().createPathGeometry();
            auto sink = geometry.open();
            sink.beginFigure({ x1, y });
            sink.addQuadraticBezier({ { x1 + dx, y - side }, { x1 + side, y } });
            sink.addQuadraticBezier({ { x1 - dx, y + side }, { x1, y } });
            sink.endFigure(FigureEnd::Closed);
            sink.close();

            g.drawGeometry(geometry, brush1, 1.0f);
        }
    }

    // Fills, then the same four brushes as strokes.
    const float width = 80.0f;

    for (int pass = 0; pass < 2; ++pass)
    {
        const bool stroking = pass == 1;
        const float y1 = stroking ? 170.0f : 90.0f;
        const float y2 = y1 + 60.0f;
        x1 = margin;

        {
            auto solidBrush = g.createSolidColorBrush(Colors::LightSeaGreen);
            if (stroking)
                g.drawRectangle({ x1, y1, x1 + width, y2 }, solidBrush, 4.0f);
            else
                g.fillRectangle({ x1, y1, x1 + width, y2 }, solidBrush);
        }
        x1 += width + margin;

        {
            Gradientstop gradientStops[] = {
                { 0.0f, Colors::Silver },
                { 1.0f, Colors::LightSlateGray }
            };
            auto collection = g.createGradientstopCollection(gradientStops);
            LinearGradientBrushProperties props{ { 0.0f, y1 }, { 0.0f, y2 } };
            auto gradientBrush = g.createLinearGradientBrush(props, {}, collection);

            const RoundedRect rr{ { x1, y1, x1 + width, y2 }, margin, margin };
            if (stroking)
                g.drawRoundedRectangle(rr, gradientBrush, 6.0f);
            else
                g.fillRoundedRectangle(rr, gradientBrush);
        }
        x1 += width + margin;

        {
            const Point gradientCenter{ x1 + width * 0.25f, y1 + (y2 - y1) * 0.25f };
            Gradientstop gradientStops[] = {
                { 0.0f, Colors::White },
                { 1.0f, Colors::Peru }
            };
            auto collection = g.createGradientstopCollection(gradientStops);
            auto gradientBrush = g.createRadialGradientBrush(collection, gradientCenter, width);

            const RoundedRect rr{ { x1, y1, x1 + width, y2 }, margin * 2.0f, margin * 2.0f };
            if (stroking)
                g.drawRoundedRectangle(rr, gradientBrush, 8.0f);
            else
                g.fillRoundedRectangle(rr, gradientBrush);
        }
        x1 += width + margin;

        {
            auto tartan = detail::makeTartan(g);
            auto bitmapBrush = g.createBitmapBrush(tartan);

            const float radius = (y2 - y1) * 0.5f;
            const Point center{ x1 + radius, y1 + radius };
            if (stroking)
                g.drawCircle(center, radius, bitmapBrush, 10.0f);
            else
                g.fillCircle(center, radius, bitmapBrush);
        }
    }

    // Alpha compositing: three half-transparent primaries over black.
    {
        const float radius = size.height * 0.06f;
        const Point p{ size.width * 0.5f, size.height - radius - margin };

        auto blackBrush = g.createSolidColorBrush(Colors::Black);
        g.fillCircle(p, radius * 1.8f, blackBrush);

        auto red   = g.createSolidColorBrush(Color{ 1.0f, 0.0f, 0.0f, 0.5f });
        auto green = g.createSolidColorBrush(Color{ 0.0f, 1.0f, 0.0f, 0.5f });
        auto blue  = g.createSolidColorBrush(Color{ 0.0f, 0.0f, 1.0f, 0.5f });

        g.fillCircle({ p.x - radius * 0.433f, p.y + radius * 0.25f }, radius, red);
        g.fillCircle({ p.x,                   p.y - radius * 0.5f  }, radius, green);
        g.fillCircle({ p.x + radius * 0.433f, p.y + radius * 0.25f }, radius, blue);
    }
}

} // namespace demo
