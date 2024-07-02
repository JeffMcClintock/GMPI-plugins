#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include "helpers/GmpiPluginEditor.h"

using namespace gmpi;
using namespace gmpi::drawing;

void drawKnob(Graphics& g, const Rect& bounds, float pinGain)
{
	auto r = bounds;

	auto center = Point{ (r.left + r.right) * 0.5f, (r.top + r.bottom) * 0.5f };
	auto radius = (std::min)(r.right, r.bottom) * 0.4f;
	auto thickness = radius * 0.2f;

	auto brush = g.createSolidColorBrush(Colors::Chocolate);

	// inner Gray circle
	auto dimBrush = g.createSolidColorBrush(Colors::LightSlateGray);

	const float startAngle = 35.0f; // angle between "straight-down" and start of arc. In degrees.
	const float startAngleRadians = startAngle * M_PI / 180.f; // angle between "straight-down" and start of arc. In degrees.
	const float quarterTurnClockwise = M_PI * 0.5f;

	Point startPoint{ center.x + radius * cosf(quarterTurnClockwise + startAngleRadians), center.y + radius * sinf(quarterTurnClockwise + startAngleRadians) };
	StrokeStyleProperties strokeStyleProperties;
	strokeStyleProperties.lineCap = CapStyle::Round;
	auto strokeStyle = g.getFactory().createStrokeStyle(strokeStyleProperties);

	// Background gray arc.
	{
		float sweepAngle = (M_PI * 2.0f - startAngleRadians * 2.0f);

		Point endPoint{ center.x + radius * cosf(quarterTurnClockwise + startAngleRadians + sweepAngle), center.y + radius * sinf(quarterTurnClockwise + startAngleRadians + sweepAngle) };

		auto arcGeometry = g.getFactory().createPathGeometry();
		auto sink = arcGeometry.open();
		sink.beginFigure(startPoint);
		sink.addArc(ArcSegment{ endPoint, Size{ radius, radius }, 0.0f, SweepDirection::Clockwise, sweepAngle > M_PI ? ArcSize::Large : ArcSize::Small });
		sink.endFigure(FigureEnd::Open);
		sink.close();

		g.drawGeometry(arcGeometry, dimBrush, thickness, strokeStyle);
	}

	// foreground coloured arc
	{
		float nomalised = pinGain;
		float sweepAngle = nomalised * (static_cast<float>(M_PI) * 2.0f - startAngleRadians * 2.0f);

		Point endPoint{ center.x + radius * cosf(quarterTurnClockwise + startAngleRadians + sweepAngle), center.y + radius * sinf(quarterTurnClockwise + startAngleRadians + sweepAngle) };

		auto arcGeometry = g.getFactory().createPathGeometry();
		auto sink = arcGeometry.open();
		sink.beginFigure(startPoint);
		sink.addArc(ArcSegment{ endPoint, Size{radius, radius}, 0.0f, SweepDirection::Clockwise, sweepAngle > M_PI ? ArcSize::Large : ArcSize::Small });
		sink.endFigure(FigureEnd::Open);
		sink.close();

		g.drawGeometry(arcGeometry, brush, thickness, strokeStyle);
	}
}
