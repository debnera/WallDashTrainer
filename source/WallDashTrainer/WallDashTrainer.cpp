#include "pch.h"
#include "WallDashTrainer.h"

#include <sstream>
#include <set>


BAKKESMOD_PLUGIN(WallDashTrainer, "WallDashTrainer", plugin_version, PLUGINTYPE_FREEPLAY);

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

float WallDashTrainer::GetCurrentTime()
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

void WallDashTrainer::onLoad()
{
	// This line is required for `LOG` to work and must be before any use of `LOG()`.
	_globalCvarManager = cvarManager;

	persistentStorage = std::make_shared<PersistentStorage>(this, "wall_dash_trainer", true, true);
	
	auto registerIntCvar = [this](std::string label, int& value)
		{
			persistentStorage->RegisterPersistentCvar(label, std::to_string(value), "", true)
				.addOnValueChanged([&](std::string oldValue, CVarWrapper cvar) { value = cvar.getIntValue(); });
		};
	auto registerFloatCvar = [this](std::string label, float& value)
		{
			persistentStorage->RegisterPersistentCvar(label, std::to_string(value), "", true)
				.addOnValueChanged([&](std::string oldValue, CVarWrapper cvar) { value = cvar.getFloatValue(); });
		};
	auto registerPercentCvar = [this](std::string label, float& value)
		{
			persistentStorage->RegisterPersistentCvar(label, std::to_string(value), "", true, true, 0.0f, true, 1.0f)
				.addOnValueChanged([&](std::string oldValue, CVarWrapper cvar) { value = cvar.getFloatValue(); });
		};
	auto registerBoolCvar = [this](std::string label, bool& value)
		{
			persistentStorage->RegisterPersistentCvar(label, std::to_string(value), "", true)
				.addOnValueChanged([&](std::string oldValue, CVarWrapper cvar) { value = cvar.getBoolValue(); });
		};
	auto registerColorCvar = [this](std::string label, LinearColor& value)
		{
			persistentStorage->RegisterPersistentCvar(label, to_string(value), "", true)
				.addOnValueChanged([&](std::string oldValue, CVarWrapper cvar) { value = cvar.getColorValue(); });
		};
	auto registerRangeListCvar = [this](std::string label, RangeList& rangeList)
		{
			persistentStorage->RegisterPersistentCvar(label, rangeList.ValuesToString(), "", true)
				.addOnValueChanged([&](std::string oldValue, CVarWrapper cvar)
					{
						auto values = RangeList::SplitString(cvar.getStringValue());
						rangeList.UpdateValues(values);
					}
				);
		};

	registerBoolCvar(PLUGIN_ENABLED, PluginEnabled);
	registerPercentCvar(GUI_POSITION_RELATIVE_X, GuiPositionRelative.X);
	registerPercentCvar(GUI_POSITION_RELATIVE_Y, GuiPositionRelative.Y);
	registerFloatCvar(GUI_SIZE, GuiSize);
	registerPercentCvar(GUI_PREVIEW_OPACTIY, GuiColorPreviewOpacity);
	registerBoolCvar(GUI_SHOW_FIRST_JUMP, GuiShowFirstJump);
	registerBoolCvar(GUI_SHOW_DOUBLE_JUMP, GuiShowDoubleJump);
	registerColorCvar(GUI_BORDER_COLOR, GuiColorBorder);
	registerColorCvar(GUI_BACKGROUND_COLOR, GuiColorBackground);
	registerColorCvar(GUI_BACKDROP_COLOR, GuiColorBackdrop);
	registerColorCvar(GUI_COLOR_SUCCESS, GuiColorSuccess);
	registerColorCvar(GUI_COLOR_WARNING, GuiColorWarning);
	registerColorCvar(GUI_COLOR_FAILURE, GuiColorFailure);
	registerRangeListCvar(GUI_JUMP_RANGES, JumpDurationRanges);
	registerRangeListCvar(GUI_DOUBLE_JUMP_RANGES, DoubleJumpDurationRanges);

	gameWrapper->RegisterDrawable(
		[this](CanvasWrapper canvas)
		{
			if (!IsActive()) return;

			WallDashTrainer::RenderCanvas(canvas);
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

			WallDashTrainer::OnTick(car, input);
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
		}
	);

	auto OnSecondJump = [this](CarComponentWrapper component)
		{
			if (IsInReplay || !IsActive()) return;
			if (!IsLocalCar(component.GetCar())) return;

			// Only register the double jump if we didn't loose our flip or landed in between.
			if (DoubleJumpPossible)
				DoubleJumpPressedTime = GetCurrentTime();

			DoubleJumpPossible = false;
		};

	// Double jump
	gameWrapper->HookEventWithCaller<CarComponentWrapper>(
		"Function CarComponent_DoubleJump_TA.Active.BeginState",
		[this, OnSecondJump](CarComponentWrapper component, ...)
		{
			OnSecondJump(component);
		}
	);

	// Dodge
	gameWrapper->HookEventWithCaller<CarComponentWrapper>(
		"Function CarComponent_Dodge_TA.Active.BeginState",
		[this, OnSecondJump](CarComponentWrapper component, ...)
		{
			// Treat dodging the same as double-jumping, since players might dodge by accident.
			OnSecondJump(component);
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

bool WallDashTrainer::IsActive()
{
	return PluginEnabled
		&& !gameWrapper->IsPaused()
		&& (gameWrapper->IsInFreeplay() || gameWrapper->IsInCustomTraining());
}

bool WallDashTrainer::IsLocalCar(CarWrapper car)
{
	CarWrapper localCar = gameWrapper->GetLocalCar();
	return car.memory_address == localCar.memory_address;
}

void WallDashTrainer::OnTick(CarWrapper car, ControllerInput* input)
{
	float now = GetCurrentTime();

	// Timer 1 shows how long we are holding the initial jump
	if (HoldingFirstJump)
		HoldFirstJumpDuration = now - HoldFirstJumpStartTime;
	else if (HoldFirstJumpStopTime > 0)
		HoldFirstJumpDuration = HoldFirstJumpStopTime - HoldFirstJumpStartTime;

	// Timer 2 shows the duration between starting the initial jump and starting the second jump
	if (HoldingFirstJump && DoubleJumpPossible)
		// Waiting for the double jump
		TimeBetweenFirstAndDoubleJump = now - HoldFirstJumpStartTime;
	else if (DoubleJumpPressedTime > HoldFirstJumpStopTime)
		// We performed our double jump
		TimeBetweenFirstAndDoubleJump = DoubleJumpPressedTime - HoldFirstJumpStartTime;
	else
		// We missed our opportunity to perform a double jump
		TimeBetweenFirstAndDoubleJump = 0;
}

static std::string toPrecision(float x, int precision)
{
	std::stringstream stream;
	stream << std::fixed << std::setprecision(precision) << x;
	return stream.str();
}

void WallDashTrainer::RenderCanvas(CanvasWrapper canvas)
{
	ScreenSize = canvas.GetSize();

	Vector2F position = GuiPosition();

	{
		Vector2F margin = { 0.02f, 0.04f };
		Vector2F size = Vector2F{ GuiSize, GuiHeight };
		canvas.SetPosition(position - (size * margin));
		canvas.SetColor(GuiColorBackdrop);
		canvas.FillBox(size * ((margin * 2.f) + 1.f));
	}

	if (GuiShowFirstJump)
	{
		DrawBar(
			canvas, "Hold First Jump: ", HoldFirstJumpDuration * 1000,
			position, BarSize(),
			GuiColorBackground, JumpDurationRanges
		);
		position += Offset();
	}

	if (GuiShowDoubleJump)
	{
		DrawBar(
			canvas, "Time Between Jumps: ", TimeBetweenFirstAndDoubleJump * 1000,
			position, BarSize(),
			GuiColorBackground, DoubleJumpDurationRanges
		);
		position += Offset();
	}
	GuiHeight = position.Y - GuiPosition().Y;
}

void WallDashTrainer::DrawBar(
	CanvasWrapper& canvas, std::string text, float value,
	Vector2F barPos, Vector2F barSize,
	LinearColor backgroundColor, RangeList& colorRanges
)
{
	if (colorRanges.IsEmpty())
		return;

	float minValue = colorRanges.GetTotalMin();
	float maxValue = colorRanges.GetTotalMax();
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
	canvas.SetColor(*colorRanges.GetColorForValue(value));
	canvas.SetPosition(barPos);
	canvas.FillBox(Vector2F{ valueToPosition(value), barSize.Y });

	// Draw separators
	// `DrawBox()` always draws lines with width `2`.
	// By offsetting the box by one pixel (half the line width), we avoid gaps and overlaps.
	auto offset = 1;
	canvas.SetColor(GuiColorBorder);
	canvas.SetPosition(barPos - offset);
	canvas.DrawBox(barSize + (2 * offset));

	auto values = colorRanges.GetValues();
	std::set<float> uniqueValues = std::set<float>(values.begin(), values.end());
	for (float value : uniqueValues)
	{
		if (value <= minValue || maxValue <= value) continue;

		auto start = barPos + Vector2F{ valueToPosition(value), 0 };
		auto end = start + Vector2F{ 0, barSize.Y };
		canvas.SetColor(GuiColorBorder);
		canvas.DrawLine(start, end, 2); // Line width `2` to match `DrawBox()`.
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


void WallDashTrainer::onUnload()
{
	// nothing to unload...
}
