/* Copyright (c) 2007-2025 SynthEdit Ltd

Permission to use, copy, modify, and /or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS.IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "helpers/GmpiPluginEditor.h"
#include "./drawSlider.h"

using namespace gmpi;
using namespace gmpi::editor;
using namespace gmpi::drawing;

typedef Pin<float> FloatPin;

struct knobInfo
{
    gmpi::drawing::Rect rect;
    std::string label;
    FloatPin& pin;
};

class SawDemoGui final : public PluginEditor
{
    Point lastMouse{};

    // Parameters
    Pin<float> unisonCount;
    Pin<float> unisonSpread;
    Pin<float> oscDetune;
    Pin<float> ampAttack;
    Pin<float> ampRelease;
    Pin<float> ampIsGate;
    Pin<float> preFilterVCA;
    Pin<float> cutoff;
    Pin<float> resonance;
    Pin<float> filterMode;

    // host information
    Pin<float> bpm;
    Pin<float> qnp;
    Pin<int32_t> numerator;
    Pin<int32_t> denominator;
    Pin<bool> bypass;

    std::vector<knobInfo> knobs;
    knobInfo* draggingKnob{};

  public:
    SawDemoGui() :
        knobs {
              {{}, "Uni Ct", unisonCount},
              {{}, "Spread", unisonSpread},
              {{}, "Detune", oscDetune},
              {{}, "VCA", preFilterVCA},
              {{}, "Cuttoff", cutoff},
              {{}, "Res", resonance},
              {{}, "Attack", ampAttack},
              {{}, "Release", ampRelease},
              {{}, "Flt Mde", filterMode},
          }
    {
        // rough layout
        float x = 30.0f;
        float y = 160.0f;
        int i = 0;
        for (auto &knob : knobs)
        {
            knob.rect = {
                x - 12.0f, // left
                y - 90.f,  // top
                x + 12.0f, // right
                y + 90.f   // bottom
            };

            x += 70.0f;

            ++i;
            if (i == 3)
            {
                x += 20.0f;
                y += 100.0f;
            }

            if (i == 6)
            {
                x = 30.0f;
                y += 120.0f;
            }
        }

        // redraw when any parameter changes.
        for (auto &knob : knobs)
        {
            knob.pin.onUpdate = [this](PinBase *) { drawingHost->invalidateRect(nullptr); };
        }

        // redraw when any time info changes.
        PinBase* timePins[] = {&bpm, &qnp, &numerator, &denominator, &bypass};
        for (PinBase *pin : timePins)
        {
            pin->onUpdate = [this](PinBase *) { drawingHost->invalidateRect(nullptr); };
        }
    }

    ReturnCode measure(const gmpi::drawing::Size *availableSize,
                       gmpi::drawing::Size *returnDesiredSize) override
    {
        *returnDesiredSize = {430,570};
        return ReturnCode::Ok;
    }

    ReturnCode render(gmpi::drawing::api::IDeviceContext *drawingContext) override
    {
        Graphics g(drawingContext);

        ClipDrawingToBounds _(g, bounds);

        auto font = g.getFactory().createTextFormat(24.0f);
        font.setTextAlignment(TextAlignment::Center);

        auto smallFont = g.getFactory().createTextFormat(12.0f);
        smallFont.setTextAlignment(TextAlignment::Center);
        smallFont.setParagraphAlignment(ParagraphAlignment::Center);

        // background color
        auto brush = g.createSolidColorBrush(colorFromHex(0xff404090));
        g.fillRectangle(bounds, brush);

        // slider area background
        brush.setColor(colorFromHex(0xff202050));
        auto bodyRect = bounds;
        bodyRect.top += 60;
        bodyRect.bottom -= 60;
        g.fillRectangle(bodyRect, brush);

        // Header text
        auto headerRect = bounds;
        headerRect.bottom = headerRect.top + 60;
        headerRect.top += 6.0f;

        brush.setColor(Colors::White);
        g.drawTextU("GMPI Saw Synth Demo", font, headerRect, brush);

        headerRect.top += 12.0f;
        std::string headerText =
            "tempo=" + std::to_string(bpm.value) +
            " ts=" + std::to_string(numerator.value) + "/" + std::to_string(denominator.value) +
            " songpos=" + std::to_string(qnp.value);
//            " poly=" + std::to_string(unisonCount.value);
        g.drawTextU(headerText, smallFont, headerRect, brush);

        // footer text
        auto footerRect = bounds;
        footerRect.top = footerRect.bottom - 50;
        g.drawTextU(
            "https://github.com/JeffMcClintock/GMPI-plugins\n\n"
                    "MIT License; GMPI 2.0"
            , smallFont, footerRect, brush);

        // Draw each knob
        for (auto &knob : knobs)
        {
            const auto normalised = knob.pin.value;
            drawSlider(g, knob.rect, normalised, knob.label);
        }

        // main outline
        g.drawRectangle(bounds, brush);

        // bypass indicator
        brush.setColor(bypass.value ? gmpi::drawing::Colors::Red : gmpi::drawing::Colors::Green);
        Ellipse circle{{26.0f, 26.0f}, 10.0f, 10.0f};
        g.fillEllipse(circle, brush);
        brush.setColor(gmpi::drawing::Colors::White);
        g.drawEllipse(circle, brush, 0.5f);

        return ReturnCode::Ok;
    }

    ReturnCode onPointerDown(Point point, int32_t flags) override
    {
        lastMouse = point;

        draggingKnob = {};
        for (auto &knob : knobs)
        {
            if (PointInRect(point, knob.rect))
            {
                draggingKnob = &knob;
            }
        }

        return inputHost->setCapture();
    }

    ReturnCode onPointerMove(Point point, int32_t flags) override
    {
        bool isCaptured = false;
        inputHost->getCapture(isCaptured);

        if (draggingKnob)
        {
            draggingKnob->pin =
                std::clamp(draggingKnob->pin.value + (lastMouse.y - point.y) * 0.005f, 0.0f, 1.0f);
        }
        lastMouse = point;
        return ReturnCode::Ok;
    }

    ReturnCode onPointerUp(Point point, int32_t flags) override
    {
        draggingKnob = {};
        return inputHost->releaseCapture();
    }
};

namespace
{
auto r = Register<SawDemoGui>::withId("GMPI: SawDemo");
}
