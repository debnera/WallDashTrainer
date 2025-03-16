#include "pch.h"
#include "FastAerialTrainer.h"

#include <sstream>
#include <set>


BAKKESMOD_PLUGIN(FastAerialTrainer, "FastAerialTrainer", plugin_version, PLUGINTYPE_FREEPLAY);

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

float FastAerialTrainer::GetCurrentTime()
{
	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (!server) return 0;

	return server.GetSecondsElapsed();
}

static std::string to_string(LinearColor col)
{
	return "("
		+ std::to_string((int)col.R) + ","
		+ std::to_string((int)col.G) + ","
		+ std::to_string((int)col.B) + ","
		+ std::to_string((int)col.A) + ")";
}

void FastAerialTrainer::onLoad()
{
	// This line is required for `LOG` to work and must be before any use of `LOG()`.
	_globalCvarManager = cvarManager;

	persistentStorage = std::make_shared<PersistentStorage>(this, "fast_aerial_trainer", true, true);

	auto registerIntCvar = [this](std::string label, int& value)
		{
			persistentStorage->RegisterPersistentCvar(label, std::to_string(value), "", false)
				.addOnValueChanged([&](std::string oldValue, CVarWrapper cvar) { value = cvar.getIntValue(); });
		};
	auto registerFloatCvar = [this](std::string label, float& value)
		{
			persistentStorage->RegisterPersistentCvar(label, std::to_string(value), "", false)
				.addOnValueChanged([&](std::string oldValue, CVarWrapper cvar) { value = cvar.getFloatValue(); });
		};
	auto registerPercentCvar = [this](std::string label, float& value)
		{
			persistentStorage->RegisterPersistentCvar(label, std::to_string(value), "", false, true, 0.0f, true, 1.0f)
				.addOnValueChanged([&](std::string oldValue, CVarWrapper cvar) { value = cvar.getFloatValue(); });
		};
	auto registerBoolCvar = [this](std::string label, bool& value)
		{
			persistentStorage->RegisterPersistentCvar(label, std::to_string(value), "", false)
				.addOnValueChanged([&](std::string oldValue, CVarWrapper cvar) { value = cvar.getBoolValue(); });
		};
	auto registerColorCvar = [this](std::string label, LinearColor& value)
		{
			persistentStorage->RegisterPersistentCvar(label, to_string(value), "", false)
				.addOnValueChanged([&](std::string oldValue, CVarWrapper cvar) { value = cvar.getColorValue(); });
		};

	registerBoolCvar(PLUGIN_ENABLED, PluginEnabled);
	registerFloatCvar(RECORD_AFTER_DOUBLE_JUMP, RecordingAfterDoubleJump);
	registerPercentCvar(GUI_POSITION_RELATIVE_X, GuiPositionRelative.X);
	registerPercentCvar(GUI_POSITION_RELATIVE_Y, GuiPositionRelative.Y);
	registerFloatCvar(GUI_SIZE, GuiSize);
	registerPercentCvar(GUI_PREVIEW_OPACTIY, GuiColorPreviewOpacity);
	registerBoolCvar(GUI_DRAW_PITCH_HISTORY, GuiDrawPitchHistory);
	registerBoolCvar(GUI_DRAW_BOOST_HISTORY, GuiDrawBoostHistory);
	registerBoolCvar(GUI_SHOW_FIRST_INPUT_WARNING, GuiShowFirstInputWarning);
	registerColorCvar(GUI_BORDER_COLOR, GuiColorBorder);
	registerColorCvar(GUI_BACKGROUND_COLOR, GuiColorBackground);
	registerColorCvar(GUI_COLOR_SUCCESS, GuiColorSuccess);
	registerColorCvar(GUI_COLOR_WARNING, GuiColorWarning);
	registerColorCvar(GUI_COLOR_FAILURE, GuiColorFailure);
	registerColorCvar(GUI_COLOR_HISTORY, GuiPitchHistoryColor);
	persistentStorage->RegisterPersistentCvar(GUI_JUMP_RANGES, JumpDurationRanges.ValuesToString(), "", false)
		.addOnValueChanged([&](std::string oldValue, CVarWrapper cvar)
			{
				auto values = RangeList::SplitString(cvar.getStringValue());
				JumpDurationRanges.UpdateValues(values);
			});
	persistentStorage->RegisterPersistentCvar(GUI_DOUBLE_JUMP_RANGES, DoubleJumpDurationRanges.ValuesToString(), "", false)
		.addOnValueChanged([&](std::string oldValue, CVarWrapper cvar)
			{
				auto values = RangeList::SplitString(cvar.getStringValue());
				DoubleJumpDurationRanges.UpdateValues(values);
			});

	gameWrapper->RegisterDrawable(
		[this](CanvasWrapper canvas)
		{
			if (!IsActive()) return;

			FastAerialTrainer::RenderCanvas(canvas);
		}
	);

	gameWrapper->HookEventWithCaller<CarWrapper>(
		"Function TAGame.Car_TA.SetVehicleInput",
		[this](CarWrapper car, void* params, std::string eventName)
		{
			if (IsInReplay || !IsActive()) return;
			if (!IsLocalCar(car)) return;

			auto input = static_cast<ControllerInput*>(params);
			if (!input) return;

			FastAerialTrainer::OnTick(car, input);
		}
	);

	// Initial jump start
	gameWrapper->HookEventWithCaller<CarComponentWrapper>(
		"Function CarComponent_Jump_TA.Active.BeginState",
		[this](CarComponentWrapper component, void* params, std::string eventName)
		{
			if (IsInReplay || !IsActive()) return;
			if (!IsLocalCar(component.GetCar())) return;

			float now = GetCurrentTime();

			HoldingFirstJump = true;
			HoldFirstJumpStartTime = now;
			HoldFirstJumpStopTime = 0;
			HoldFirstJumpDuration = 0;

			DoubleJumpPossible = true;
			DoubleJumpPressedTime = 0;
			TimeBetweenFirstAndDoubleJump = 0;
			TotalRecordingDuration = 0;
			HoldingJoystickBackDuration = 0;

			LastTickTime = now;
			InputHistory.clear();
		}
	);

	// Double jump
	gameWrapper->HookEventWithCaller<CarComponentWrapper>(
		"Function CarComponent_DoubleJump_TA.Active.BeginState",
		[this](CarComponentWrapper component, void* params, std::string eventName)
		{
			if (IsInReplay || !IsActive()) return;
			if (!IsLocalCar(component.GetCar())) return;

			// Only register the double jump if we didn't loose our flip or landed in between.
			if (DoubleJumpPossible)
				DoubleJumpPressedTime = GetCurrentTime();

			DoubleJumpPossible = false;
		}
	);

	// Jump released
	gameWrapper->HookEventWithCaller<CarWrapper>(
		"Function TAGame.Car_TA.OnJumpReleased",
		[this](CarWrapper car, void* params, std::string eventName)
		{
			if (IsInReplay || !IsActive()) return;
			if (!IsLocalCar(car)) return;

			if (HoldingFirstJump)
			{
				HoldingFirstJump = false;
				HoldFirstJumpStopTime = GetCurrentTime();
			}
		}
	);

	// Are we in a replay?
	gameWrapper->HookEvent(
		"Function GameEvent_Soccar_TA.ReplayPlayback.BeginState",
		[this](...) { IsInReplay = true; }
	);
	gameWrapper->HookEvent(
		"Function GameEvent_Soccar_TA.ReplayPlayback.EndState",
		[this](...) { IsInReplay = false; }
	);

	// Freeplay or training start/reset
	gameWrapper->HookEvent(
		"Function GameEvent_Soccar_TA.Countdown.BeginState",
		[this](...)
		{
			HoldingFirstJump = false;
			DoubleJumpPossible = false;
		}
	);

	// Custom training attempt started
	gameWrapper->HookEvent(
		"Function TAGame.TrainingEditorMetrics_TA.TrainingShotAttempt",
		[this](...)
		{
			TrainingStartTime = GetCurrentTime();
		}
	);
}

bool FastAerialTrainer::IsActive()
{
	return PluginEnabled
		&& !gameWrapper->IsPaused()
		&& (gameWrapper->IsInFreeplay() || gameWrapper->IsInCustomTraining());
}

bool FastAerialTrainer::IsLocalCar(CarWrapper car)
{
	CarWrapper localCar = gameWrapper->GetLocalCar();
	return car.memory_address == localCar.memory_address;
}

void FastAerialTrainer::OnTick(CarWrapper car, ControllerInput* input)
{
	float now = GetCurrentTime();

	if (HoldingFirstJump)
		HoldFirstJumpDuration = now - HoldFirstJumpStartTime;
	else if (HoldFirstJumpStopTime > 0)
		HoldFirstJumpDuration = HoldFirstJumpStopTime - HoldFirstJumpStartTime;

	if (HoldingFirstJump)
		TimeBetweenFirstAndDoubleJump = 0;
	else if (DoubleJumpPressedTime > HoldFirstJumpStopTime)
		TimeBetweenFirstAndDoubleJump = DoubleJumpPressedTime - HoldFirstJumpStopTime;
	else if (DoubleJumpPossible)
		TimeBetweenFirstAndDoubleJump = now - HoldFirstJumpStopTime;
	else
		TimeBetweenFirstAndDoubleJump = 0;

	// We either landed or have to land for another double jump. No need to record things further.
	if (!car.HasFlip() || (car.IsOnGround() && !car.GetbJumped()))
		DoubleJumpPossible = false;

	auto InAfterDoubleJumpRecording = DoubleJumpPressedTime <= now
		&& now <= DoubleJumpPressedTime + RecordingAfterDoubleJump;

	if (DoubleJumpPossible || InAfterDoubleJumpRecording)
	{
		float sensitivity = gameWrapper->GetSettings().GetGamepadSettings().AirControlSensitivity;
		float intensity = std::min(1.f, sensitivity * input->Pitch);
		float duration = now - LastTickTime;
		HoldingJoystickBackDuration += intensity * duration;
		TotalRecordingDuration += duration;
		LastTickTime = now;

		InputHistory.push_back({ intensity, (bool)input->HoldingBoost, (bool)input->Jumped });
	}
}

static std::string toPrecision(float x, int precision)
{
	std::stringstream stream;
	stream << std::fixed << std::setprecision(precision) << x;
	return stream.str();
}

void FastAerialTrainer::RenderCanvas(CanvasWrapper canvas)
{
	ScreenSize = canvas.GetSize();

	DrawBar(
		canvas, "Hold First Jump: ", HoldFirstJumpDuration * 1000,
		GuiPosition(), BarSize(),
		GuiColorBackground, JumpDurationRanges
	);

	DrawBar(
		canvas, "Time to Double Jump: ", TimeBetweenFirstAndDoubleJump * 1000,
		GuiPosition() + Offset(), BarSize(),
		GuiColorBackground, DoubleJumpDurationRanges
	);

	canvas.SetColor(GuiColorBorder);
	canvas.SetPosition(GuiPosition() + (Offset() * 2));
	float JoystickBackDurationPercentage = !TotalRecordingDuration ? 0.f : 100.f * HoldingJoystickBackDuration / TotalRecordingDuration;
	canvas.DrawString("Pitch Up Amount: " + toPrecision(JoystickBackDurationPercentage, 1) + "%", FontSize(), FontSize());

	if (GuiDrawPitchHistory)
		DrawPitchHistory(canvas);

	if (GuiDrawBoostHistory)
		DrawBoostHistory(canvas);

	if (GuiShowFirstInputWarning)
		RenderFirstInputWarning(canvas);
}

void FastAerialTrainer::DrawBar(
	CanvasWrapper& canvas, std::string text, float value,
	Vector2F barPos, Vector2F barSize,
	LinearColor backgroundColor, RangeList& colorRanges
)
{
	if (colorRanges.GetRanges().empty())
		return;

	float minValue = colorRanges.GetRanges().front().min;
	float maxValue = colorRanges.GetRanges().back().max;
	auto valueToPosition = [&](float value)
		{
			auto ratio = (value - minValue) / (maxValue - minValue);
			return barSize.X * std::clamp(ratio, 0.f, 1.f);
		};

	// Draw background
	canvas.SetPosition(barPos);
	canvas.SetColor(backgroundColor);
	canvas.FillBox(barSize);

	for (Range& range : colorRanges.GetRanges())
	{
		LinearColor preview = *range.color;
		preview.A *= GuiColorPreviewOpacity;
		canvas.SetColor(preview);

		float left = valueToPosition(range.min);
		float right = valueToPosition(range.max);
		float width = right - left;
		canvas.SetPosition(barPos + Vector2F{ left, 0 });
		canvas.FillBox(Vector2F{ width, barSize.Y });
	}

	// Draw colored bar
	if (value < minValue)
		canvas.SetColor(*colorRanges.GetRanges().front().color);
	else if (value >= maxValue)
		canvas.SetColor(*colorRanges.GetRanges().back().color);
	else
	{
		for (Range& range : colorRanges.GetRanges())
		{
			if (range.min <= value && value < range.max)
				canvas.SetColor(*range.color);
		}
	}
	canvas.SetPosition(barPos);
	canvas.FillBox(Vector2F{ valueToPosition(value), barSize.Y });

	// Draw separators
	// `DrawBox()` always draws lines with width `2`.
	// By offsetting the box by one pixel (half the line width), we avoid gaps and overlaps.
	auto offset = 1;
	canvas.SetColor(GuiColorBorder);
	canvas.SetPosition(barPos - offset);
	canvas.DrawBox(barSize + (2 * offset));

	std::set<float> values;
	for (Range& range : colorRanges.GetRanges()) values.insert({ range.min, range.max });
	for (float value : values)
	{
		if (minValue < value && value < maxValue)
		{
			canvas.SetColor(GuiColorBorder);
			auto start = barPos + Vector2F{ valueToPosition(value), 0 };
			auto end = start + Vector2F{ 0, barSize.Y };
			canvas.DrawLine(start, end, 2); // Line width `2` to match `DrawBox()`.
		}
	}

	// Draw text
	canvas.SetColor(GuiColorBorder);
	canvas.SetPosition(barPos + Vector2F{ 0, barSize.Y });
	canvas.DrawString(text + toPrecision(value, 1) + " ms", FontSize(), FontSize());
}

static void DrawCenteredText(CanvasWrapper canvas, std::string text, float fontSize, Vector2F topLeft, Vector2F boxSize)
{
	Vector2F textSize = canvas.GetStringSize(text, fontSize, fontSize);
	canvas.SetPosition(topLeft + (boxSize - textSize) / 2);
	canvas.DrawString(text, fontSize, fontSize);
}

void FastAerialTrainer::DrawPitchHistory(CanvasWrapper& canvas)
{
	float borderWidth = 2;
	float textWidth = 45 * FontSize();
	Vector2F topLeft = GuiPosition() + (Offset() * 2.6f) + Vector2F{ borderWidth, 0 };
	Vector2F innerBoxSize = Vector2F{ GuiSize, GuiSize / 10 };

	canvas.SetColor(GuiColorBorder);
	DrawCenteredText(
		canvas,
		"Pitch",
		FontSize(),
		topLeft + Vector2F{ innerBoxSize.X - textWidth, 0 },
		Vector2F{ textWidth, innerBoxSize.Y }
	);
	innerBoxSize -= Vector2F{ textWidth, 0 };

	canvas.SetColor(GuiColorBorder);
	canvas.SetPosition(topLeft - borderWidth);
	canvas.DrawBox(innerBoxSize + (2 * borderWidth));

	int size = (int)InputHistory.size();
	int i = 0;
	InputHistoryItem previousInput{};
	for (InputHistoryItem currentInput : InputHistory)
	{
		if (i > 0)
		{
			float startX = (float)(i - 1) / (size - 1);
			float endX = (float)i / (size - 1);

			float startY = 1.f - std::clamp(previousInput.pitch, 0.f, 1.f);
			float endY = 1.f - std::clamp(currentInput.pitch, 0.f, 1.f);

			// Draw a right triangle from `start` to `end` and a rectangle beneath it.
			Vector2F start = innerBoxSize * Vector2F{ startX, startY };
			Vector2F end = innerBoxSize * Vector2F{ endX, endY };
			Vector2F base = start.Y > end.Y ? Vector2F{ end.X, start.Y } : Vector2F{ start.X, end.Y };

			canvas.SetColor(GuiPitchHistoryColor); // Note: `FillTriangle` ignores transparency.
			canvas.FillTriangle(base + topLeft, start + topLeft, end + topLeft);
			canvas.SetPosition(Vector2F{ start.X, std::max(start.Y, end.Y) } + topLeft);
			canvas.FillBox(Vector2F{ end.X - start.X, innerBoxSize.Y - std::max(start.Y, end.Y) });
		}
		previousInput = currentInput;
		i++;
	}

	i = 0;
	for (InputHistoryItem input : InputHistory)
	{
		if (input.jumped)
		{
			canvas.SetColor(GuiColorBorder);
			Vector2F start = topLeft + Vector2F{ (float)i / size * innerBoxSize.X, 0.f };
			Vector2F end = start + Vector2F{ 0, innerBoxSize.Y };
			canvas.DrawLine(start, start + Vector2F{ 0,innerBoxSize.Y }, 2 * borderWidth);
		}
		i++;
	}
}

void FastAerialTrainer::DrawBoostHistory(CanvasWrapper& canvas)
{
	float borderWidth = 2;
	float textWidth = 45 * FontSize();
	Vector2F offset = Offset() * (GuiDrawPitchHistory ? 3.8f : 2.6f);
	Vector2F topLeft = GuiPosition() + offset + Vector2F{ borderWidth, 0 };
	Vector2F innerBoxSize = BarSize();

	canvas.SetColor(GuiColorBorder);
	DrawCenteredText(
		canvas,
		"Boost",
		FontSize(),
		topLeft + Vector2F{ innerBoxSize.X - textWidth, 0 },
		Vector2F{ textWidth, innerBoxSize.Y }
	);
	innerBoxSize -= Vector2F{ textWidth, 0 };

	canvas.SetColor(GuiColorBorder);
	canvas.SetPosition(topLeft - borderWidth);
	canvas.DrawBox(innerBoxSize + (2 * borderWidth));

	int i = 0;
	int size = (int)InputHistory.size();
	for (InputHistoryItem input : InputHistory)
	{
		if (input.boost)
		{
			canvas.SetColor(GuiPitchHistoryColor);
			canvas.SetPosition(topLeft + innerBoxSize * Vector2F{ (float)i / size, 0 });
			canvas.FillBox(innerBoxSize * Vector2F{ 1.f / size, 1.f });
		}
		i++;
	}
}

void FastAerialTrainer::RenderFirstInputWarning(CanvasWrapper& canvas)
{
	if (!gameWrapper->IsInCustomTraining()) return;
	if (TrainingStartTime >= HoldFirstJumpStartTime) return;

	float offset = 2.6f;
	if (GuiDrawPitchHistory) offset += 1.2f;
	if (GuiDrawBoostHistory) offset += 0.6f;

	Vector2F position = GuiPosition() + Offset() * offset;

	canvas.SetColor(GuiColorBorder);
	canvas.SetPosition(position);
	canvas.DrawString("Jump was not first input!", FontSize(), FontSize());
}

void FastAerialTrainer::onUnload()
{
	// nothing to unload...
}

RangeList::RangeList(std::vector<float> values, std::vector<LinearColor*> colors)
{
	if (colors.size() != values.size() - 1)
		LOG("Constructing RangeList: Number of values and colors don't match!");

	for (int i = 0; i < std::min(colors.size(), values.size() - 1); i++)
	{
		ranges.push_back(
			{
				.min = values[i],
				.max = values[i + 1],
				.color = colors[i]
			}
		);
	}
}

void RangeList::UpdateValues(std::vector<float> values)
{
	auto size = std::min(values.size(), ranges.size() + 1);
	for (int i = 0; i < size; i++)
	{
		if (i > 0) ranges[i - 1].max = values[i];
		if (i < size - 1) ranges[i].min = values[i];
	}
}
std::vector<Range>& RangeList::GetRanges()
{
	return ranges;
}

std::string RangeList::ValuesToString()
{
	std::string result;

	if (ranges.empty())
		return result;

	result = std::to_string(ranges[0].min);

	for (auto& range : ranges)
		result += "," + std::to_string(range.max);

	return result;
}
std::vector<float> RangeList::SplitString(std::string str)
{
	std::vector<float> values;
	std::istringstream stream(str);
	std::string value;

	while (std::getline(stream, value, ','))
		values.push_back(strtof(value.c_str(), NULL));

	return values;
}