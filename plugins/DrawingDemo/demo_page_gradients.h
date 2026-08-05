#pragma once

// Colour-space and banding checks.
//
// Every row draws the SAME content twice: once straight to the screen render
// target, once into an offscreen WIC bitmap (SRGBPixels | CpuReadable) that is
// then blitted back. The two halves sit side by side, so a colour-space mistake
// on either path shows as a visible seam down the middle, rather than as an
// absolute brightness you would have to measure to judge.
//
// What each row is for:
//
//   sRGB ramp            column x is sRGB grey x. Perceptually even — the
//                        reference for what "correct" looks like.
//   Linear ramp          column x is LINEAR intensity x/255. Correct output is
//                        bright and washed out, with mid grey landing at sRGB
//                        188. If this looks like the row above, something is
//                        writing linear values into an 8-bit buffer and
//                        showing them raw.
//   Gradient brush       black -> white through the gradient brush, which
//                        interpolates linearly, so its midpoint is 188 too.
//   Dark gradient        black -> 0.05 linear. Nothing here is about hue: this
//                        row exists to be BANDED. Eight bits spent linearly
//                        leave about 13 distinct levels across this range
//                        instead of about 64, which reads as wide steps.
//                        Smooth means the 8-bit face is sRGB-encoded.
//   Dither vs solid      a 1-pixel black/white dither whose DENSITY is the
//                        linear intensity. It matches the solid linear ramp
//                        only if compositing is linear-correct, because the
//                        dither averages physically.
//   Lock-written bitmap  left is the sRGB ramp drawn with brushes, right is a
//                        createImage() bitmap with byte x written straight into
//                        the locked pixels. Checks that a CPU-written bitmap is
//                        read back in the space it was written in.
//
// Headless coverage of the same contract lives in gimpi_ui_tests
// (CpuVsD2D.SRGBRenderTargetRoundTripIsFaithful and friends). This page is the
// end-to-end version: it also exercises the swap chain and the display.

#include <cstdint>
#include <functional>
#include <vector>

#include "GmpiUiDrawing.h"

namespace demo
{

// Paints a ramp into `area`. BitmapRenderTarget derives from Graphics, so the
// same call works against the screen and against an offscreen bitmap — which is
// exactly what makes the two comparable.
using RampFn = std::function<void(gmpi::drawing::Graphics&, gmpi::drawing::Rect area)>;

namespace detail
{

// Solid 1-pixel columns rather than a gradient brush, so nothing but the colour
// value itself is under test.
inline void columnRamp(gmpi::drawing::Graphics& g, gmpi::drawing::Rect area,
                       const std::function<gmpi::drawing::Color(int)>& colorAt)
{
    auto brush = g.createSolidColorBrush(gmpi::drawing::Colors::Black);
    const float dx = (area.right - area.left) / 256.0f;
    for (int x = 0; x < 256; ++x)
    {
        brush.setColor(colorAt(x));
        g.fillRectangle({ area.left + x * dx, area.top,
                          area.left + (x + 1) * dx, area.bottom }, brush);
    }
}

inline void srgbRamp(gmpi::drawing::Graphics& g, gmpi::drawing::Rect area)
{
    columnRamp(g, area, [](int x) {
        const auto v = static_cast<uint8_t>(x);
        return gmpi::drawing::colorFromSrgba(v, v, v);
    });
}

inline void linearRamp(gmpi::drawing::Graphics& g, gmpi::drawing::Rect area)
{
    columnRamp(g, area, [](int x) {
        const float v = x / 255.0f;
        return gmpi::drawing::Color{ v, v, v, 1.0f };
    });
}

inline void gradientRamp(gmpi::drawing::Graphics& g, gmpi::drawing::Rect area,
                         gmpi::drawing::Color to)
{
    gmpi::drawing::Gradientstop stops[] = {
        { 0.0f, gmpi::drawing::Colors::Black },
        { 1.0f, to }
    };
    auto collection = g.createGradientstopCollection(stops);
    gmpi::drawing::LinearGradientBrushProperties props{
        { area.left, area.top }, { area.right, area.top } };
    auto brush = g.createLinearGradientBrush(props, {}, collection);
    g.fillRectangle(area, brush);
}

inline void gradientRampWhite(gmpi::drawing::Graphics& g, gmpi::drawing::Rect area)
{
    gradientRamp(g, area, gmpi::drawing::Colors::White);
}

// 0 -> 0.05 linear. Deliberately much darker than the "dark half" the other
// rows use: banding only bites where 8-bit linear has run out of levels, and
// this is that range.
inline void gradientRampDark(gmpi::drawing::Graphics& g, gmpi::drawing::Rect area)
{
    gradientRamp(g, area, gmpi::drawing::Color{ 0.05f, 0.05f, 0.05f, 1.0f });
}

// Ordered dither. Deterministic on purpose: a rand()-based pattern shimmers on
// every repaint, which makes a still comparison impossible.
inline void ditheredLinearRamp(gmpi::drawing::Graphics& g, gmpi::drawing::Rect area)
{
    static constexpr int bayer[4][4] = {
        {  0,  8,  2, 10 },
        { 12,  4, 14,  6 },
        {  3, 11,  1,  9 },
        { 15,  7, 13,  5 }
    };

    auto black = g.createSolidColorBrush(gmpi::drawing::Colors::Black);
    auto white = g.createSolidColorBrush(gmpi::drawing::Colors::White);
    g.fillRectangle(area, black);

    const float dx   = (area.right - area.left) / 256.0f;
    const int   rows = static_cast<int>(area.bottom - area.top);

    for (int x = 0; x < 256; ++x)
    {
        // Density is the LINEAR intensity, because a black/white dither averages
        // physically (linearly) once displayed.
        const float density = x / 255.0f;
        for (int y = 0; y < rows; ++y)
        {
            const float threshold = (bayer[y & 3][x & 3] + 0.5f) / 16.0f;
            if (density > threshold)
            {
                g.fillRectangle({ area.left + x * dx,       area.top + y,
                                  area.left + (x + 1) * dx, area.top + y + 1.0f }, white);
            }
        }
    }
}

// A bitmap whose bytes the CPU writes, rather than the rasterizer. Byte x in all
// three colour channels: if the lock format means what its name says, this is
// sRGB grey x and matches the brush-drawn sRGB ramp exactly.
inline gmpi::drawing::Bitmap lockWrittenRamp(gmpi::drawing::Graphics& g, int w, int h)
{
    auto bitmap = g.getFactory().createImage(
        gmpi::drawing::SizeU{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) },
        static_cast<int32_t>(gmpi::drawing::BitmapRenderTargetFlags::SRGBPixels));

    auto pixels = bitmap.lockPixels(gmpi::drawing::BitmapLockFlags::Write);
    if (!pixels)
        return bitmap;

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const auto v = static_cast<uint8_t>((x * 256) / w);
            // Channel order comes from the bitmap itself. Backends do not agree
            // on it, and assuming BGRA because of _WIN32 is how this gets
            // silently swapped somewhere else.
            pixels.setPixel(x, y, pixels.rgBytesToPixel(v, v, v));
        }
    }
    return bitmap;
}

} // namespace detail

class GradientsPage
{
    // One column per 8-bit code: any level the format cannot represent shows up
    // as a repeated column, which is what banding actually is.
    static constexpr int   kRampW  = 256;
    static constexpr float kRowH   = 40.0f;
    static constexpr float kLabelH = 16.0f;
    static constexpr float kGapY   = 8.0f;
    static constexpr float kGapX   = 16.0f;
    static constexpr float kMargin = 12.0f;
    static constexpr float kHeadH  = 22.0f;

    struct Row
    {
        const char* label;
        RampFn      paint;
    };

    // Offscreen copies, built once. Caching is not an optimisation here — a
    // cached bitmap drawn again later is precisely the case that used to come
    // back darker than the direct draw.
    std::vector<gmpi::drawing::Bitmap> cache;

    static const std::vector<Row>& rows()
    {
        static const std::vector<Row> r{
            { "sRGB ramp — the reference: evenly spaced to the eye",           detail::srgbRamp },
            { "Linear ramp — correct is WASHED OUT, mid grey at sRGB 188",     detail::linearRamp },
            { "Gradient brush, black to white (interpolates linearly)",        detail::gradientRampWhite },
            { "Dark gradient to 0.05 linear — BANDING probe, must be smooth",  detail::gradientRampDark },
            { "Dither, density = linear intensity — should match row 2",       detail::ditheredLinearRamp },
        };
        return r;
    }

public:
    static constexpr float preferredWidth  = kMargin * 2 + kRampW * 2 + kGapX;
    static constexpr float preferredHeight =
        kMargin * 2 + kHeadH + 6 * (kLabelH + kRowH + kGapY);

    // Device-dependent resources: drop them when the device is lost.
    void invalidateCache() { cache.clear(); }

    void draw(gmpi::drawing::Graphics& g, gmpi::drawing::Size size)
    {
        using namespace gmpi::drawing;

        g.clear(colorFromHex(0x1E1E1Eu));

        auto textBrush = g.createSolidColorBrush(Colors::WhiteSmoke);
        auto dimBrush  = g.createSolidColorBrush(Colors::DarkGray);
        auto font      = g.getFactory().createTextFormat(12.0f);

        const float leftX  = kMargin;
        const float rightX = kMargin + kRampW + kGapX;

        {
            Rect r{ leftX, kMargin, leftX + kRampW, kMargin + kHeadH };
            g.drawTextU("DIRECT TO SCREEN", font, r, textBrush);
            g.drawTextU("VIA WIC BITMAP — must look identical",
                        font, offsetRect(r, { kRampW + kGapX, 0.0f }), textBrush);
        }

        if (cache.empty())
        {
            for (const auto& row : rows())
                cache.push_back(renderOffscreen(g, row.paint));
        }

        float y = kMargin + kHeadH;
        size_t idx = 0;

        for (const auto& row : rows())
        {
            g.drawTextU(row.label, font,
                        Rect{ leftX, y, size.width - kMargin, y + kLabelH }, dimBrush);
            y += kLabelH;

            row.paint(g, Rect{ leftX, y, leftX + kRampW, y + kRowH });

            // Blitted 1:1 with nearest neighbour, so the blit itself cannot
            // smooth over a difference.
            if (idx < cache.size() && cache[idx])
            {
                g.drawBitmap(cache[idx],
                             Rect{ rightX, y, rightX + kRampW, y + kRowH },
                             Rect{ 0.0f, 0.0f, static_cast<float>(kRampW), kRowH },
                             1.0f, BitmapInterpolationMode::NearestNeighbor);
            }

            y += kRowH + kGapY;
            ++idx;
        }

        // Last row: brush-drawn sRGB ramp against a CPU-lock-written bitmap.
        g.drawTextU("Lock-written bitmap — bytes written by the CPU, not the rasterizer",
                    font, Rect{ leftX, y, size.width - kMargin, y + kLabelH }, dimBrush);
        y += kLabelH;

        detail::srgbRamp(g, Rect{ leftX, y, leftX + kRampW, y + kRowH });

        if (!lockWritten)
            lockWritten = detail::lockWrittenRamp(g, kRampW, static_cast<int>(kRowH));

        if (lockWritten)
        {
            g.drawBitmap(lockWritten,
                         Rect{ rightX, y, rightX + kRampW, y + kRowH },
                         Rect{ 0.0f, 0.0f, static_cast<float>(kRampW), kRowH },
                         1.0f, BitmapInterpolationMode::NearestNeighbor);
        }
    }

private:
    gmpi::drawing::Bitmap lockWritten;

    gmpi::drawing::Bitmap renderOffscreen(gmpi::drawing::Graphics& g, const RampFn& paint) const
    {
        // SRGBPixels | CpuReadable is the WIC-bitmap path on Windows: D2D
        // rasterizes into an 8-bit surface, and drawing that result back is the
        // round trip this page exists to check.
        constexpr int32_t flags =
              static_cast<int32_t>(gmpi::drawing::BitmapRenderTargetFlags::SRGBPixels)
            | static_cast<int32_t>(gmpi::drawing::BitmapRenderTargetFlags::CpuReadable);

        auto rt = g.getFactory().createCpuRenderTarget(
            gmpi::drawing::SizeU{ static_cast<uint32_t>(kRampW),
                                  static_cast<uint32_t>(kRowH) }, flags);

        rt.beginDraw();
        rt.clear(gmpi::drawing::Colors::Black);
        paint(rt, gmpi::drawing::Rect{ 0.0f, 0.0f,
                                       static_cast<float>(kRampW), kRowH });
        rt.endDraw();

        return rt.getBitmap();
    }
};

} // namespace demo
