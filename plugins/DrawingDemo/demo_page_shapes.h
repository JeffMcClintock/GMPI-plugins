#pragma once

// Geometry, stroking and fills: line caps, dash styles, line joins, beziers,
// the four brush types (solid, linear gradient, radial gradient, bitmap) as both
// fills and strokes, and an alpha-compositing check.
//
// Ported from gmpi_ui/examples/exampleJucePlugin, so the same scene can be
// compared between the JUCE backend and the native ones.

#include <algorithm>
#include <cstdint>
#include <iterator>

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

    // LAID OUT PROPORTIONALLY. The JUCE original hard-coded pixel positions for
    // its 400x300 window, so in anything larger the whole scene huddled into the
    // top-left corner with a void beneath it. Everything below is derived from
    // `size` instead, in four bands: strokes/joins/curves, fills, outlines, and
    // the alpha-compositing circles.
    const float margin = (std::max)(14.0f, (std::min)(size.width, size.height) / 26.0f);
    const float gap    = margin;

    // Four equal columns carry the fills and outlines rows.
    const float colW = (size.width - 2.0f * margin - 3.0f * gap) / 4.0f;

    const float usableH  = size.height - 2.0f * margin;
    const float bandTopH = usableH * 0.30f;   // lines, joins, curves
    const float rowH     = usableH * 0.19f;   // fills, then outlines
    const float topY     = margin;
    const float fillsY   = topY + bandTopH + gap;
    const float strokesY = fillsY + rowH + gap;

    const detail::lineStyle lineStyles[] = {
        { CapStyle::Flat,   Colors::Salmon,        DashStyle::Solid },
        { CapStyle::Round,  Colors::DarkOrange,    DashStyle::Solid },
        { CapStyle::Square, Colors::Gold,          DashStyle::Solid },
        { CapStyle::Square, Colors::LightSeaGreen, DashStyle::Dash  },
        { CapStyle::Round,  Colors::DodgerBlue,    DashStyle::Dot   }
    };

    auto brush1 = g.createSolidColorBrush(Colors::Green);

    // Band 1, column 0: dash styles and line caps.
    {
        const float spacing = bandTopH / static_cast<float>(std::size(lineStyles) + 1);
        float y = topY + spacing;

        for (const auto& style : lineStyles)
        {
            StrokeStyleProperties strokeStyleProperties{};
            strokeStyleProperties.lineCap   = style.capStyle;
            strokeStyleProperties.dashStyle = style.dashStyle;

            auto strokeStyle = g.getFactory().createStrokeStyle(strokeStyleProperties);
            brush1.setColor(style.color);

            g.drawLine({ margin, y }, { margin + colW, y }, brush1, 6.0f, strokeStyle);
            y += spacing;
        }
    }

    // Line joins.
    const detail::triangleStyle linejoins[] = {
        { Colors::Firebrick,  LineJoin::Bevel },
        { Colors::Coral,      LineJoin::Miter },
        { Colors::Aquamarine, LineJoin::Round }
    };

    // Band 1, column 1: line joins, three triangles sharing a baseline.
    {
        const float side     = (std::min)(colW / 2.6f, bandTopH / 2.6f);
        const float baseline = topY + bandTopH * 0.72f;
        float x1 = margin + colW + gap;

        for (const auto& style : linejoins)
        {
            StrokeStyleProperties strokeStyleProperties{};
            strokeStyleProperties.lineJoin = style.lineJoin;

            auto strokeStyle = g.getFactory().createStrokeStyle(strokeStyleProperties);

            auto geometry = g.getFactory().createPathGeometry();
            auto sink = geometry.open();
            sink.beginFigure({ x1, baseline });
            sink.addLine({ x1 + side, baseline });
            sink.addLine({ x1 + side * 0.5f, baseline - side * 0.866f });
            sink.endFigure(FigureEnd::Closed);
            sink.close();

            brush1.setColor(style.color);
            g.drawGeometry(geometry, brush1, 6.0f, strokeStyle);

            x1 += side * 1.4f;
        }
    }

    // Band 1, columns 2-3: a fan of quadratic beziers.
    {
        const float side = (std::min)(colW, bandTopH * 0.5f);
        const float cx   = margin + 2.0f * (colW + gap) + colW;
        const float cy   = topY + bandTopH * 0.5f;

        brush1.setColor(Colors::GreenYellow);
        for (float dx = -side; dx < side; dx += side / 7.0f)
        {
            auto geometry = g.getFactory().createPathGeometry();
            auto sink = geometry.open();
            sink.beginFigure({ cx, cy });
            sink.addQuadraticBezier({ { cx + dx, cy - side }, { cx + side, cy } });
            sink.addQuadraticBezier({ { cx - dx, cy + side }, { cx, cy } });
            sink.endFigure(FigureEnd::Closed);
            sink.close();

            g.drawGeometry(geometry, brush1, 1.0f);
        }
    }

    // Bands 2 and 3: the four brush types, first as fills then as outlines, one
    // brush per column so the two rows line up for comparison.
    const float width = colW;

    for (int pass = 0; pass < 2; ++pass)
    {
        const bool  stroking = pass == 1;
        const float y1 = stroking ? strokesY : fillsY;
        const float y2 = y1 + rowH;
        float x1 = margin;

        {
            auto solidBrush = g.createSolidColorBrush(Colors::LightSeaGreen);
            if (stroking)
                g.drawRectangle({ x1, y1, x1 + width, y2 }, solidBrush, 4.0f);
            else
                g.fillRectangle({ x1, y1, x1 + width, y2 }, solidBrush);
        }
        x1 += width + gap;

        {
            Gradientstop gradientStops[] = {
                { 0.0f, Colors::Silver },
                { 1.0f, Colors::LightSlateGray }
            };
            auto collection = g.createGradientstopCollection(gradientStops);
            LinearGradientBrushProperties props{ { 0.0f, y1 }, { 0.0f, y2 } };
            auto gradientBrush = g.createLinearGradientBrush(props, {}, collection);

            const RoundedRect rr{ { x1, y1, x1 + width, y2 }, rowH * 0.2f, rowH * 0.2f };
            if (stroking)
                g.drawRoundedRectangle(rr, gradientBrush, 6.0f);
            else
                g.fillRoundedRectangle(rr, gradientBrush);
        }
        x1 += width + gap;

        {
            const Point gradientCenter{ x1 + width * 0.25f, y1 + (y2 - y1) * 0.25f };
            Gradientstop gradientStops[] = {
                { 0.0f, Colors::White },
                { 1.0f, Colors::Peru }
            };
            auto collection = g.createGradientstopCollection(gradientStops);
            auto gradientBrush = g.createRadialGradientBrush(collection, gradientCenter, width);

            const RoundedRect rr{ { x1, y1, x1 + width, y2 }, rowH * 0.4f, rowH * 0.4f };
            if (stroking)
                g.drawRoundedRectangle(rr, gradientBrush, 8.0f);
            else
                g.fillRoundedRectangle(rr, gradientBrush);
        }
        x1 += width + gap;

        {
            auto tartan = detail::makeTartan(g);
            auto bitmapBrush = g.createBitmapBrush(tartan);

            // Centred in its column, not pinned to the left of it, so the four
            // brushes read as one grid.
            const float radius = (std::min)(colW, y2 - y1) * 0.5f;
            const Point center{ x1 + width * 0.5f, (y1 + y2) * 0.5f };
            if (stroking)
                g.drawCircle(center, radius, bitmapBrush, 10.0f);
            else
                g.fillCircle(center, radius, bitmapBrush);
        }
    }

    // Band 4: alpha compositing, three half-transparent primaries over black.
    // Centred in whatever is left below the outlines row rather than pinned to
    // the bottom edge — pinning left it stranded far below everything else.
    {
        const float bandTop = strokesY + rowH + gap;
        const float bandH   = (std::max)(0.0f, size.height - margin - bandTop);
        const float radius  = (std::min)(size.height * 0.06f, bandH * 0.42f);
        const Point p{ size.width * 0.5f, bandTop + bandH * 0.5f };

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
