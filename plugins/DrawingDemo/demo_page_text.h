#pragma once

// Text: weights, styles, stretches, a gradient-filled string, wrapped paragraph
// text, colour emoji and fallback, and the alignment matrix.
//
// Ported from gmpi_ui/examples/exampleJucePlugin, so the same scene can be
// compared between the JUCE backend and the native ones. Text is the least
// portable part of any drawing API, which is what makes it worth a page.

#include <algorithm>
#include <array>
#include <string_view>

#include "GmpiUiDrawing.h"

namespace demo
{

namespace detail
{

struct fontOptions
{
    std::string_view          name;
    gmpi::drawing::FontWeight weight  = gmpi::drawing::FontWeight::Normal;
    gmpi::drawing::FontStyle  style   = gmpi::drawing::FontStyle::Normal;
    gmpi::drawing::FontStretch stretch = gmpi::drawing::FontStretch::Normal;
};

struct fontAlignments
{
    std::string_view                  name;
    gmpi::drawing::TextAlignment      alignment;
    gmpi::drawing::ParagraphAlignment paragraphAlignment;
};

} // namespace detail

inline void drawTextPage(gmpi::drawing::Graphics& g, gmpi::drawing::Size size)
{
    using namespace gmpi::drawing;

    g.clear(colorFromHex(0x323E44u));

    const detail::fontOptions options[] = {
        { "Regular",   FontWeight::Normal, FontStyle::Normal, FontStretch::Normal },
        { "Bold",      FontWeight::Bold,   FontStyle::Normal, FontStretch::Normal },
        { "Italic",    FontWeight::Normal, FontStyle::Italic, FontStretch::Normal },
        { "Condensed", FontWeight::Normal, FontStyle::Normal, FontStretch::Condensed },
        { "Gradient",  FontWeight::Normal, FontStyle::Normal, FontStretch::Normal },
    };

    // Height alone sized this in the JUCE original, whose window was 400 wide.
    // In a wider-but-shorter window the five style samples then overflow the
    // right edge, so cap on width too -- the row is ~22 ems of text.
    const float textheight = (std::min)(size.height / 12.0f, size.width / 22.0f);
    const float margin     = textheight / 2.0f;

    Rect textRect{ margin, margin, margin, margin + textheight };

    auto brush = g.createSolidColorBrush(Colors::WhiteSmoke);

    for (const auto& option : options)
    {
        // Deliberately a font that does not exist, to exercise fallback.
        std::array<std::string_view, 1> fontFamilies{ "Segoe" };

        auto font = g.getFactory().createTextFormat(
            textheight, fontFamilies, option.weight, option.style, option.stretch);

        const auto textSize = font.getTextExtentU(option.name);
        textRect.right = textRect.left + textSize.width + 1;

        if (option.name == "Gradient")
        {
            Gradientstop gradientStops[] = {
                { 0.0f, Colors::Violet },
                { 1.0f, Colors::MediumPurple }
            };
            auto collection = g.createGradientstopCollection(gradientStops);
            LinearGradientBrushProperties props{ { 0.0f, textRect.top }, { 0.0f, textRect.bottom } };
            auto gradientBrush = g.createLinearGradientBrush(props, {}, collection);

            g.drawTextU(option.name, font, textRect, gradientBrush);
        }
        else
        {
            g.drawTextU(option.name, font, textRect, brush);
        }

        textRect = offsetRect(textRect, { 0.5f * margin + getWidth(textRect), 0.0f });
    }

    // Wrapped paragraph.
    {
        const auto text =
            "One spring morning at four o'clock the first cuckoo arrived in the valley "
            "of the Moomins. He perched on the blue roof of Moomin house and cuckooed 8 "
            "times - rather hoarsely to be sure, for it was still a bit early in the "
            "spring.\n   Then he flew away to the east.\n   Moomintroll woke up and lay "
            "a long time looking at the ceiling before he realised where he was.";

        auto font = g.getFactory().createTextFormat(14.0f);

        textRect.left   = margin;
        textRect.right  = size.width / 2.0f;
        textRect.top    = textRect.bottom + margin;
        textRect.bottom = size.height - margin;

        g.drawTextU(text, font, textRect, brush);
    }

    // Colour emoji and script fallback.
    //
    // Spelled with \u escapes rather than literal characters on purpose. Pasted
    // CJK and emoji only survive if the file is saved as UTF-8 WITH a BOM --
    // without one MSVC decodes the bytes as the system codepage and the string
    // reaches the renderer as mojibake, which looks like a font-fallback bug and
    // is not one. Escapes are encoding-independent, so the file can be plain
    // UTF-8 and this still says what it means:
    //   \u7D75\u6587\u5B57 = "emoji" in Japanese, exercising CJK fallback
    //   \U0001F991 squid, \U0001F600 grinning face = colour-glyph fallback
    {
        const auto text = reinterpret_cast<const char*>(
            u8"Color emoji: \u7D75\u6587\u5B57 \U0001F991 \U0001F600");

        auto font = g.getFactory().createTextFormat(14.0f);
        const auto textSize = font.getTextExtentU(text);

        textRect.left  = textRect.right + margin;
        textRect.right = size.width - margin;
        g.drawTextU(text, font, textRect, brush);

        textRect.top = textRect.top + textSize.height + margin;
    }

    // Alignment matrix.
    {
        const detail::fontAlignments alignments[] = {
            { "Left",     TextAlignment::Leading,  ParagraphAlignment::Center },
            { "Center",   TextAlignment::Center,   ParagraphAlignment::Center },
            { "Right",    TextAlignment::Trailing, ParagraphAlignment::Center },
            { "Top",      TextAlignment::Center,   ParagraphAlignment::Near   },
            { "Bottom",   TextAlignment::Center,   ParagraphAlignment::Far    },
            { "Baseline", TextAlignment::Center,   ParagraphAlignment::Far    },
        };

        const float boxHeight = 28.0f;
        auto font = g.getFactory().createTextFormat(14.0f);

        Rect alignmentRect{ textRect.left, textRect.top, textRect.right, textRect.top + boxHeight };
        auto boxBrush = g.createSolidColorBrush(Colors::Blue);

        for (const auto& box : alignments)
        {
            g.drawRectangle(alignmentRect, boxBrush);

            font.setTextAlignment(box.alignment);
            font.setParagraphAlignment(box.paragraphAlignment);

            if (box.name == "Baseline")
            {
                const auto metrics = font.getFontMetrics();

                auto r = alignmentRect;
                r.top = r.bottom - calcBodyHeight(metrics);
                r = offsetRect(r, { 0.0f, metrics.descent });
                g.drawTextU(box.name, font, r, brush);
            }
            else
            {
                g.drawTextU(box.name, font, alignmentRect, brush);
            }

            alignmentRect = offsetRect(alignmentRect, { 0.0f, boxHeight + 2.0f });
        }
    }
}

} // namespace demo
