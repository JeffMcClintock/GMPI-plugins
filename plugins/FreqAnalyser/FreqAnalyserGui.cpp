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
#include "helpers/Timer.h"
#include "helpers/SimplifyGraph.h"

using namespace gmpi;
using namespace gmpi::editor;
using namespace gmpi::drawing;

class FreqAnalyserGui final : public PluginEditor, public gmpi::TimerClient
{
	Pin<Blob> pinSpectrum;
	Pin<int32_t> pinMode;
	Pin<int32_t> pindBHigh;
	Pin<int32_t> pindBLow;

	const float leftBorder = 22.0f;
	float rightBorder = 0;

	gmpi::drawing::Bitmap cachedBackground_;
	float GraphXAxisYcoord;
	float currentBackgroundSampleRate = 0.f;
	gmpi::drawing::PathGeometry peakPath;
	gmpi::drawing::PathGeometry geometry;
	gmpi::drawing::PathGeometry lineGeometry;
	int peakResetCounter = 0;

	int smoothedZoneHigh = 0; // bins below this need 4-point interpolation
	int linearZoneHigh = 0; // bins below this need 2-point interpolation

	std::vector<float> dbs_in;   // latest values from the Processor
	std::vector<float> dbs_disp; // values to display, will decay over time
	std::vector<bool> dbUsed;

	struct binData
	{
		int index;
		float fraction;
		float hz;
	};

	std::vector< binData > pixelToBin;
	std::vector<int> closestPixelToBin;

	//	float pixelToBinDx = 2.0f; // x increment for each entry in pixelToBin.
	float dbToPixel = 1.0f;
	std::vector<float> rawSpectrum;
	float sampleRateFft = 0;
	bool spectrumUpdateFlag = true;
	bool spectrumDecayFlag = true;

	std::vector<gmpi::drawing::Point> graphValues;
	std::vector<gmpi::drawing::Point> graphValuesOptimized;
	std::vector<gmpi::drawing::Point> peakHoldValues;
	std::vector<gmpi::drawing::Point> peakHoldValuesOptimized;

	void updatePaths(const std::vector<gmpi::drawing::Point>& graphValuesOptimized, const std::vector<gmpi::drawing::Point>& peakHoldValuesOptimized)// override
	{
		geometry = {};
		peakPath = {};
		drawingHost->invalidateRect({});
	}

	void InixPixelToBin(std::vector<binData>& pixelToBin, int graphWidth, int numValues, float sampleRate)// override
	{
		pixelToBin.clear();

		const float hz2bin = numValues / (0.5f * sampleRate);

		const float totalWidth = graphWidth;
		const float lowHz = 20.0f;
		const int interpolationExtraValues = 3;
		const int numBars = interpolationExtraValues + static_cast<int>(totalWidth);

		int section = 0;
		int x = 0;
		for (int j = 0; j < numBars; ++j)
		{
			const float octave = x * 10.0f / totalWidth;
			const float hz = powf(2.0f, octave) * lowHz;

			const float bin = hz * hz2bin;
			const float safeBin = std::clamp(bin, 0.f, (float)numValues - 1.0f);

			const int index = static_cast<int>(safeBin);
			const float fraction = safeBin - (float)index;
			pixelToBin.push_back(
				{
					index
					,fraction
					,hz
				}
			);

			++x;
		}
	}

	void updateSpectrumGraph(int width, int height) {
		if (rawSpectrum.size() < 10 || sampleRateFft < 1.0f)
			return;

		const int spectrumCount2 = static_cast<int>(rawSpectrum.size()) - 1;

		if (pixelToBin.empty())
		{
			InixPixelToBin(pixelToBin, width, spectrumCount2, sampleRateFft);

			// calc the zones of the graph that need cubic/linear/none interpolation
			smoothedZoneHigh = linearZoneHigh = pixelToBin.size() - 1;

			float bin_per_pixel = 0.8f;
			for (int i = 10; i < pixelToBin.size(); i++)
			{
				const auto& e1 = pixelToBin[i - 1];
				const auto& e2 = pixelToBin[i];
				const float b1 = e1.index + e1.fraction;
				const float b2 = e2.index + e2.fraction;
				const float distance = b2 - b1;

				if (distance >= bin_per_pixel)
				{
					// _RPTN(0, "bin_per_pixel %d: %d\n", (int)bin_per_pixel, i);
					if (bin_per_pixel <= 1.0f)
					{
						smoothedZoneHigh = i;
					}
					else
					{
						linearZoneHigh = i;
						break;
					}

					++bin_per_pixel;
				}
			}

			// for the right side of graph, clump bins together. Assigning each to it's nearest pixel.
			{
				const float bin2Hz = sampleRateFft / (2.0f * spectrumCount2);
				closestPixelToBin.assign(spectrumCount2, 0);

				int closestX = 0;
				for (int bin = 0; bin < spectrumCount2; bin++)
				{
					const float hz = bin * bin2Hz;

					float closest = 1000000;
					for (int x = closestX; x < pixelToBin.size(); ++x)
					{
						const auto dist = fabs(pixelToBin[x].hz - hz);
						if (dist < closest)
						{
							closestX = x;
							closest = dist;
						}
						else if (pixelToBin[x].hz > hz) // break early rather than racing off in the wrong direction to the end of the array
						{
							break;
						}
					}

					// quantize to 2 pixels in x direction.
					const auto closestXQuantized = ((closestX - linearZoneHigh) & ~1) + linearZoneHigh;
					closestPixelToBin[bin] = closestXQuantized;
				}
			}

			// mark all bins required in final graph. saves calcing db then discarding it.
			dbUsed.assign(spectrumCount2 + 2, false);
			for (int i = 0; i < pixelToBin.size(); ++i) // refactor into 3 loops
			{
				auto& index = pixelToBin[i].index;

				dbUsed[index] = true;
				if (index > smoothedZoneHigh)
				{
					dbUsed[index + 1] = true;
				}
				else
				{
					dbUsed[(std::max)(0, index - 1)] = true;
					dbUsed[index + 1] = true;
					dbUsed[index + 2] = true;
				}
			}
		}

		constexpr float displayDbTop = 6.0f;
		constexpr float displayDbBot = -120.0f;
		constexpr float clipDbAtBottom = displayDbBot - 5.0f; // -5 to have flat graph just off the bottom
		const float inverseN = 2.0f / spectrumCount2;
		const float dbc = 20.0f * log10f(inverseN);
		const float safeMinAmp = powf(10.0f, (clipDbAtBottom - dbc) * 0.1f);
		const float* capturedata = rawSpectrum.data();

		if (spectrumUpdateFlag)
		{
			dbToPixel = -height / (displayDbTop - displayDbBot);

			// convert spectrum to dB
			if (dbs_in.size() != spectrumCount2 + 2)
			{
				dbs_in.assign(spectrumCount2 + 2, -300.0f);
				dbs_disp.assign(spectrumCount2 + 2, -300.0f);
			}

			for (int i = 0; i < pixelToBin[linearZoneHigh].index; ++i)
			{
				if (!dbUsed[i])
					continue;

				float db;
				if (capturedata[i] <= safeMinAmp)
				{
					// save on expensive log10 call if signal is so quiet that it's off the bottom of the graph anyhow.
					db = clipDbAtBottom;
				}
				else
				{
					// 10 is wrong? should be 20????
					db = 10.f * log10(capturedata[i]) + dbc;
					assert(!isnan(db));
				}

				dbs_in[i] = db;
				dbs_disp[i] = (std::max)(db, dbs_disp[i]);
			}

			// for the dense part of the graph at right, just find local maxima of small groups. Then do the expensive conversion to dB.
			{
				//			int dx = 4;
				float maxAmp = 0.0f;
				int currentX = -1;
				for (int bin = 0; bin < closestPixelToBin.size(); ++bin)
				{
					int x = closestPixelToBin[bin];
					if (x != currentX)
					{
						if (currentX >= linearZoneHigh)
						{
							float db;
							if (maxAmp <= safeMinAmp)
							{
								// save on expensive log10 call if signal is so quiet that it's off the bottom of the graph anyhow.
								db = clipDbAtBottom;
							}
							else
							{
								db = 10.f * log10(maxAmp) + dbc;
								assert(!isnan(db));
							}
							const int sumaryBin = pixelToBin[currentX].index;

							dbs_in[sumaryBin] = db;
							dbs_disp[sumaryBin] = (std::max)(db, dbs_disp[sumaryBin]);
						}
						currentX = x;
						maxAmp = 0.0f;
					}

					maxAmp = (std::max)(maxAmp, capturedata[bin]);
				}
			}
		}

		graphValues.clear();

		// calc the leftmost cubic-smoothed section

		int dx = 2;
#ifdef _DEBUG
		dx = 1;
#endif
		for (int x = 0; x < smoothedZoneHigh; x += dx)
		{
			const auto& [index
				, fraction
				, hz
			] = pixelToBin[x];

			assert(index >= 0);
			assert(index + 2 < dbs_in.size());

			assert(dbUsed[(std::max)(0, index - 1)] && dbUsed[index] && dbUsed[index + 1] && dbUsed[index + 2]);

			const float y0 = dbs_disp[(std::max)(0, index - 1)];
			const float y1 = dbs_disp[index + 0];
			const float y2 = dbs_disp[index + 1];
			const float y3 = dbs_disp[index + 2];

			const auto db = (y1 + 0.5f * fraction * (y2 - y0 + fraction * (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3 + fraction * (3.0f * (y1 - y2) + y3 - y0))));

			const auto y = (db - displayDbTop) * dbToPixel;

			graphValues.push_back({ static_cast<float>(x), y });
		}

		// calc the 2-point interpolated section
		for (int x = smoothedZoneHigh; x < linearZoneHigh; x += dx)
		{
			const auto& [index
				, fraction
				, hz
			] = pixelToBin[x];

			assert(index >= 0);
			assert(index + 2 < dbs_in.size());

			assert(dbUsed[index] && dbUsed[index + 1]);

			const auto& y0 = dbs_disp[index + 0];
			const auto& y1 = dbs_disp[index + 1];

			const auto db = y0 + (y1 - y0) * fraction;
			const auto y = (db - displayDbTop) * dbToPixel;

			graphValues.push_back({ static_cast<float>(x), y });
		}

		// plot the 'max' section where we just take the maximum value of several bins.
		for (int x = linearZoneHigh; x < width; x += 2) // see also closestXQuantized
		{
			const auto dbBin = pixelToBin[x].index;

			const auto db = dbs_disp[dbBin];
			const auto y = (db - displayDbTop) * dbToPixel;

			graphValues.push_back({ static_cast<float>(x), y });
		}

		// PEAK HOLD
		if (peakHoldValues.size() != graphValues.size())
		{
			peakHoldValues = graphValues;
		}
		if (spectrumUpdateFlag)
		{
			for (int j = 0; j < graphValues.size(); ++j)
			{
				peakHoldValues[j].y = (std::min)(peakHoldValues[j].y, graphValues[j].y);
			}

			SimplifyGraph(peakHoldValues, peakHoldValuesOptimized);
		}

		SimplifyGraph(graphValues, graphValuesOptimized);

		updatePaths(graphValuesOptimized, peakHoldValuesOptimized);

		spectrumUpdateFlag = false;
		spectrumDecayFlag = false;
	
	};
	void decayGraph(float dbDecay)
	{
		for (int i = 0; i < dbs_in.size(); ++i)
		{
			dbs_disp[i] = (std::max)(dbs_in[i], dbs_disp[i] - dbDecay);
		}
		spectrumDecayFlag = true;
	}
	void clearPeaks() {
		peakHoldValues.clear();
	}


 	void onSetCaptureDataA()
	{
		const auto size = pinSpectrum.value.size();

		if (size > sizeof(float))
		{
			const auto data = (float*) pinSpectrum.value.data();

			const auto newSpectrumCount = (std::max)(0, -1 + static_cast<int>(size / sizeof(float)));

			rawSpectrum.assign((float*)data, (float*)data + size / sizeof(float));
			auto newSampleRateFft = ((float*)data)[newSpectrumCount]; // last entry used for sample-rate.

			if (sampleRateFft != newSampleRateFft)
			{
				cachedBackground_ = {};
				sampleRateFft = newSampleRateFft;
				pixelToBin.clear();
				clearPeaks();
			}

			graphValues.clear();
			spectrumUpdateFlag = true;

			//	geometry = nullptr;

			if (peakResetCounter-- < 0)
			{
				clearPeaks();
				peakResetCounter = 60;
			}
		}

		drawingHost->invalidateRect({});
	}

 	void onSetMode()
	{
		// pinMode changed
	}

 	void onSetdBHigh()
	{
		// pindBHigh changed
	}

 	void onSetdBLow()
	{
		// pindBLow changed
	}

public:
	FreqAnalyserGui() = default;

	ReturnCode measure(const gmpi::drawing::Size* availableSize,
		gmpi::drawing::Size* returnDesiredSize) override
	{
		*returnDesiredSize = { 400, 200 };
		return ReturnCode::Ok;
	}

	ReturnCode arrange(const gmpi::drawing::Rect* finalRect) override
	{
		rightBorder = (finalRect->right - finalRect->left) - leftBorder * 0.5f;

		return PluginEditor::arrange(finalRect);
	}

	ReturnCode open(gmpi::api::IUnknown* host) override
	{
		pinSpectrum.onUpdate = [this](PinBase*) { onSetCaptureDataA(); };
		startTimer(50);

		return PluginEditor::open(host);
	}

	ReturnCode render(gmpi::drawing::api::IDeviceContext* drawingContext) override
	{
		Graphics g(drawingContext);

//		auto textFormat = g.getFactory().createTextFormat();
//		auto brush = g.createSolidColorBrush(Colors::Red);

//		g.drawTextU("Hello World!", textFormat, bounds, brush);

		auto r = bounds;

		ClipDrawingToBounds cd(g, r);

		constexpr float displayOctaves = 10;
		float displayDbRange = (float)(std::max)(10, pindBHigh.value - pindBLow.value);
		float displayDbMax = (float)pindBHigh.value;

		const float snapToPixelOffset = 0.5f;

		float width = r.right - r.left;
		float height = r.bottom - r.top;

		float scale = height * 0.46f;
		float mid_x = floorf(0.5f + width * 0.5f);
		float mid_y = floorf(0.5f + height * 0.5f);

		float bottomBorder = leftBorder;
		float graphWidth = rightBorder - leftBorder;

		const auto newSpectrumCount = (std::max)(0, -1 + static_cast<int>(pinSpectrum.value.size() / sizeof(float)));
		auto capturedata = (const float*)pinSpectrum.value.data();

#if 0
		if (newSpectrumCount > 0)
		{
			const auto& newSamplerate = capturedata[newSpectrumCount]; // last entry used for sample-rate.

			// Invalidate background if SRT changes.
			if (currentBackgroundSampleRate != sampleRateFft)
				cachedBackground_ = nullptr;

			if (sampleRateFft != newSamplerate || rawSpectrum.size() != newSpectrumCount)
			{
				sampleRateFft = newSamplerate;
				pixelToBin.clear();
				graphValues.clear();
			}
		}
#endif

#if USE_CACHED_BACKGROUND
		if (cachedBackground_.isNull())
		{
			auto dc = g.createCompatibleRenderTarget(Size(r.getWidth(), r.getHeight()));
			dc.BeginDraw();
#else
			{
				auto& dc = g;
#endif
				currentBackgroundSampleRate = sampleRateFft;

				auto dtextFormat = dc.getFactory().createTextFormat(10); // GetTextFormat(getHost(), getGuiHost(), "tty", &typeface_);

				dtextFormat.setTextAlignment(TextAlignment::Leading); // Left
				dtextFormat.setParagraphAlignment(ParagraphAlignment::Center);
				dtextFormat.setWordWrapping(WordWrapping::NoWrap); // prevent word wrapping into two lines that don't fit box.

				auto gradientBrush = dc.createLinearGradientBrush(Point(0, 0), Point(0, height), colorFromHex(0x39323A), colorFromHex(0x080309));
				dc.fillRectangle(r, gradientBrush);

				auto fontBrush = dc.createSolidColorBrush(Colors::Gold);
				auto brush2 = dc.createSolidColorBrush(Colors::Gray);
				float penWidth = 1.0f;
				auto fontHeight = dtextFormat.getTextExtentU("M").height;

				// dB labels.
				float lastTextY = -10;
				if (height > 30)
				{
					float db = displayDbMax;
					float y = 0;
					while (true)
					{
						y = (displayDbMax - db) * (height - bottomBorder) / displayDbRange;
						y = snapToPixelOffset + floorf(0.5f + y);

						if (y >= height - fontHeight)
						{
							break;
						}

						GraphXAxisYcoord = y;

						dc.drawLine(gmpi::drawing::Point(leftBorder, y), gmpi::drawing::Point(rightBorder, y), brush2, penWidth);

						if (y > lastTextY + fontHeight * 1.2)
						{
							lastTextY = y;
							char txt[10];
							sprintf(txt, "%3.0f", (float)db);

							//				TextOut(hDC, 0, y - fontHeight / 2, txt, (int)wcslen(txt));
							gmpi::drawing::Rect textRect(0, y - fontHeight / 2, 30, y + fontHeight / 2);
							dc.drawTextU(txt, dtextFormat, textRect, fontBrush);
						}

						db -= 10.0f;
					}

					// extra line at -3dB. To help check filter cuttoffs.
					db = -3.f;
					y = (displayDbMax - db) * (height - bottomBorder) / displayDbRange;
					y = snapToPixelOffset + floorf(0.5f + y);

					dc.drawLine(gmpi::drawing::Point(leftBorder, y), gmpi::drawing::Point(rightBorder, y), brush2, penWidth);
				}

				//		if (pinMode == 0) // Log
				{
					// FREQUENCY LABELS
					// Highest mark is nyquist rounded to nearest 10kHz.
					const float topFrequency = 20000.0f; // calcTopFrequency3(sampleRateFft);
					float frequencyStep = 1000.0;
					if (width < 500)
					{
						frequencyStep = 10000.0;
					}
					float hz = topFrequency;
					float previousTextLeft = INT_MAX;
					float x = INT_MAX;
					do {
						const float octave = displayOctaves - logf(topFrequency / hz) / logf(2.0f);

						x = leftBorder + graphWidth * octave / displayOctaves;

						// hmm, can be misleading when grid line is one pixel off due to snapping
		//				x = snapToPixelOffset + floorf(0.5f + x);

						if (x <= leftBorder || hz < 5.0)
							break;

						bool extendLine = false;

						// Text.
						if (sampleRateFft > 0.0f)
						{
							char txt[10];

							// Large values printed in kHz.
							if (hz > 999.0)
							{
								sprintf(txt, "%.0fk", hz * 0.001);
							}
							else
							{
								sprintf(txt, "%.0f", hz);
							}

							//				int stringLength = strlen(txt);
							//SIZE size;
							//::GetTextExtentPoint32(hDC, txt, stringLength, &size);
							auto size = dtextFormat.getTextExtentU(txt);
							// Ensure text don't overwrite text to it's right.
							if (x + size.width / 2 < previousTextLeft)
							{
								extendLine = true;
								//					TextOut(hDC, x, height - fontHeight, txt, stringLength);

								gmpi::drawing::Rect textRect(x - size.width / 2, height - fontHeight, x + size.width / 2, height);
								dc.drawTextU(txt, dtextFormat, textRect, fontBrush);

								previousTextLeft = x - (2 * size.width) / 3; // allow for text plus whitepace.
							}
						}

						// Vertical line.
						float lineBottom = height - fontHeight;
						if (!extendLine)
						{
							lineBottom = GraphXAxisYcoord;
						}
						dc.drawLine(gmpi::drawing::Point(x, 0), gmpi::drawing::Point(x, lineBottom), brush2, penWidth);

						//if (hz > 950 && hz < 1050)
						//{
						//	_RPTN(0, "1k line @ %f\n", x);
						//}

						if (frequencyStep > hz * 0.99f)
						{
							frequencyStep *= 0.1f;
						}

						hz = hz - frequencyStep;

					} while (true);
				}
#if 0
				else
				{
					// FREQUENCY LABELS
					// Highest mark is nyquist rounded to nearest 2kHz.
					float topFrequency = floor(samplerate / 2000.0f) * 1000.0f;
					float frequencyStep = 1000.0;
					float hz = topFrequency;
					float previousTextLeft = INT_MAX;
					float x = INT_MAX;
					do {
						x = leftBorder + (2.0f * hz * graphWidth) / samplerate;
						x = snapToPixelOffset + floorf(0.5f + x);

						if (x <= leftBorder || hz < 5.0)
							break;

						bool extendLine = false;

						// Text.
						if (samplerate > 0)
						{
							char txt[10];

							// Large values printed in kHz.
							if (hz > 999.0)
							{
								sprintf(txt, "%.0fk", hz * 0.001);
							}
							else
							{
								sprintf(txt, "%.0f", hz);
							}

							auto size = dtextFormat.getTextExtentU(txt);
							// Ensure text don't overwrite text to it's right.
							if (x + size.width / 2 < previousTextLeft)
							{
								extendLine = true;
								//					TextOut(hDC, x, height - fontHeight, txt, stringLength);

								gmpi::drawing::Rect textRect(x - size.width / 2, height - fontHeight, x + size.width / 2, height);
								dc.drawTextU(txt, dtextFormat, textRect, fontBrush);

								previousTextLeft = x - (2 * size.width) / 3; // allow for text plus whitepace.
							}
						}

						// Vertical line.
						float lineBottom = height - fontHeight;
						if (!extendLine)
						{
							lineBottom = GraphXAxisYcoord;
						}
						dc.drawLine(gmpi::drawing::Point(x, 0), gmpi::drawing::Point(x, lineBottom), brush2, penWidth);

						hz = hz - frequencyStep;

					} while (true);
				}
#endif
#if USE_CACHED_BACKGROUND
				dc.endDraw();
				cachedBackground_ = dc.GetBitmap();
#endif
			}

#if USE_CACHED_BACKGROUND
			g.drawBitmap(cachedBackground_, Point(0, 0), r);
#endif

#if 0
			if (graphValues.empty())
			{
				rawSpectrum.assign(capturedata, capturedata + newSpectrumCount);
				const auto plotAreaWidth = rightBorder - leftBorder;
				updateSpectrumGraph(plotAreaWidth, r.getHeight());
			}
#endif

			if (spectrumDecayFlag || spectrumUpdateFlag)
			{
				const auto plotAreaWidth = rightBorder - leftBorder;
				updateSpectrumGraph(plotAreaWidth, getHeight(r));
			}

			if (!geometry && !graphValuesOptimized.empty())
			{
				auto factory = g.getFactory();

				geometry = factory.createPathGeometry();
				auto sink = geometry.open();

				lineGeometry = factory.createPathGeometry();
				auto lineSink = lineGeometry.open();

				peakPath = factory.createPathGeometry();
				auto peakSink = peakPath.open();

				sink.beginFigure(leftBorder, GraphXAxisYcoord, FigureBegin::Filled);
				float inverseN = 2.0f / rawSpectrum.size();
				const float dbc = 20.0f * log10f(inverseN);
				// Log
				{
					graphValues.clear();

					graphValues.push_back({ leftBorder, 0.0f }); // we will update y once we have it

					assert(graphValuesOptimized.size() > 1);

					// Main graph
					bool first = true;
					for (auto& p : graphValuesOptimized)
					{
						Point point(p.x + leftBorder, p.y);

						if (first)
						{
							lineSink.beginFigure(point);
							first = false;
						}
						else
						{
							lineSink.addLine(point);
						}

						sink.addLine(point);
					}

					// complete filled area down to axis
					sink.addLine(gmpi::drawing::Point(graphValuesOptimized.back().x + leftBorder, GraphXAxisYcoord));

					// peak graph
					first = true;
					for (auto& p : peakHoldValuesOptimized)
					{
						Point point(p.x + leftBorder, p.y);
						if (first)
						{
							peakSink.beginFigure(point);
							first = false;
						}
						else
						{
							peakSink.addLine(point);
						}
					}
				}

				lineSink.endFigure(FigureEnd::Open);
				lineSink.close();
				peakSink.endFigure(FigureEnd::Open);
				peakSink.close();
				sink.endFigure();
				sink.close();
			}

			if (geometry && lineGeometry)
			{
				auto peakColor = colorFromHex(0xFF45A1A1);
				auto peakBrush = g.createSolidColorBrush(peakColor);

				auto graphColor = colorFromHex(0xFF65B1D1);
				auto brush2 = g.createSolidColorBrush(graphColor);
				const float penWidth = 1;
				Color fill = colorFromHex(0xc08BA7BF);

				auto plotClip = r;
				plotClip.left = leftBorder;
				plotClip.bottom = GraphXAxisYcoord;
				ClipDrawingToBounds cd2(g, plotClip);

				auto fillBrush = g.createSolidColorBrush(fill);
				g.fillGeometry(geometry, fillBrush);
				g.drawGeometry(lineGeometry, brush2, penWidth);
				g.drawGeometry(peakPath, peakBrush, penWidth);
			}

		return ReturnCode::Ok;
	}

	bool onTimer() override
	{
		constexpr float dbDecay = 4.f;
		decayGraph(dbDecay);

		drawingHost->invalidateRect({});

		return true; // keep timer going.
	}

};

namespace
{
auto r = Register<FreqAnalyserGui>::withId("GMPI: Freq Analyser");
}
