#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include "helpers/GmpiPluginEditor.h"

using namespace gmpi;
using namespace gmpi::drawing;

void drawSlider(Graphics &g, const Rect &bounds, float normalised, std::string label)
{
	auto r = bounds;

	auto center = Point{ (r.left + r.right) * 0.5f, (r.top + r.bottom) * 0.5f };
	auto radius = (std::min)(r.right, r.bottom) * 0.4f;
	auto thickness = radius * 0.2f;

	auto brush = g.createSolidColorBrush(Colors::Black);

	// Fill background
	g.fillRectangle(r, brush);

    brush.setColor(Colors::White);

	const float sliderY = r.bottom - normalised * (r.bottom - r.top);

	Rect sliderRect{r};
    sliderRect.top = sliderY;
    g.fillRectangle(sliderRect, brush);

	// draw outline
    g.drawRectangle(r, brush, 0.5f);

    Rect textRect{r};
    textRect.left -= 20;
    textRect.right += 20;
    textRect.bottom = textRect.bottom + 20;
    textRect.top = textRect.bottom - 16;

	auto font = g.getFactory().createTextFormat(14.0f);

    brush.setColor(Colors::Black);
    g.fillRectangle(textRect, brush);

    brush.setColor(Colors::White);
    font.setTextAlignment(TextAlignment::Center);
    g.drawTextU(label, font, textRect, brush);
}
