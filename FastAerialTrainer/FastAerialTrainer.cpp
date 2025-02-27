#include "pch.h"
#include "FastAerialTrainer.h"
#include "bakkesmod/wrappers/Engine/WorldInfoWrapper.h"

#include <sstream>


BAKKESMOD_PLUGIN(FastAerialTrainer, "FastAerialTrainer", plugin_version, PLUGINTYPE_FREEPLAY);

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

float FastAerialTrainer::GetCurrentTime()
{
	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (!server) return 0;
	WorldInfoWrapper world = server.GetWorldInfo();
	if (!world) return 0;
	return world.GetTimeSeconds();
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
	registerIntCvar(GUI_JUMP_MAX, JumpDuration_HighestValue);
	registerIntCvar(GUI_DOUBLE_JUMP_MAX, DoubleJumpDuration_HighestValue);
	registerPercentCvar(GUI_PREVIEW_OPACTIY, GuiColorPreviewOpacity);
	registerBoolCvar(GUI_DRAW_HISTORY, GuiDrawPitchHistory);
	registerColorCvar(GUI_BORDER_COLOR, GuiColorBorder);
	registerColorCvar(GUI_BACKGROUND_COLOR, GuiColorBackground);
	registerColorCvar(GUI_COLOR_SUCCESS, GuiColorSuccess);
	registerColorCvar(GUI_COLOR_WARNING, GuiColorWarning);
	registerColorCvar(GUI_COLOR_FAILURE, GuiColorFailure);
	registerColorCvar(GUI_COLOR_HISTORY, GuiPitchHistoryColor);
	registerColorCvar(GUI_COLOR_HISTORY_BOOST, GuiPitchHistoryColorBoost);


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

			FastAerialTrainer::OnTick(car);
		}
	);

	// Initial jump start
	gameWrapper->HookEvent(
		"Function CarComponent_Jump_TA.Active.BeginState",
		[this](std::string eventname)
		{
			if (IsInReplay || !IsActive()) return;

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

	// Jump released
	gameWrapper->HookEvent(
		"Function TAGame.Car_TA.OnJumpReleased",
		[this](std::string eventname)
		{
			if (IsInReplay || !IsActive()) return;

			if (HoldingFirstJump)
			{
				HoldingFirstJump = false;
				HoldFirstJumpStopTime = GetCurrentTime();
			}
		}
	);

	// Double jump
	gameWrapper->HookEvent(
		"Function CarComponent_DoubleJump_TA.Active.BeginState",
		[this](std::string eventname)
		{
			if (IsInReplay || !IsActive()) return;

			// Only register the double jump if we didn't loose our flip or landed in between.
			if (DoubleJumpPossible)
				DoubleJumpPressedTime = GetCurrentTime();

			DoubleJumpPossible = false;
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
}

bool FastAerialTrainer::IsActive()
{
	return PluginEnabled
		&& !gameWrapper->IsPaused()
		&& (gameWrapper->IsInFreeplay() || gameWrapper->IsInCustomTraining());
}

void FastAerialTrainer::OnTick(CarWrapper car)
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

	ControllerInput input = car.GetInput();

	auto InAfterDoubleJumpRecording = DoubleJumpPressedTime <= now
		&& now <= DoubleJumpPressedTime + RecordingAfterDoubleJump;

	if (DoubleJumpPossible || InAfterDoubleJumpRecording)
	{
		float sensitivity = gameWrapper->GetSettings().GetGamepadSettings().AirControlSensitivity;
		float intensity = std::min(1.f, sensitivity * input.Pitch);
		float duration = now - LastTickTime;
		HoldingJoystickBackDuration += intensity * duration;
		TotalRecordingDuration += duration;
		LastTickTime = now;

		InputHistory.push_back({ intensity, (bool)input.HoldingBoost, (bool)input.Jumped });
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
		canvas, "Hold First Jump: ", HoldFirstJumpDuration * 1000, (float)JumpDuration_HighestValue,
		GuiPosition(), BarSize(),
		GuiColorBackground, JumpDuration_RangeList
	);

	DrawBar(
		canvas, "Time to Double Jump: ", TimeBetweenFirstAndDoubleJump * 1000, (float)DoubleJumpDuration_HighestValue,
		GuiPosition() + Offset(), BarSize(),
		GuiColorBackground, DoubleJumpDuration_RangeList
	);

	canvas.SetColor(GuiColorBorder);
	canvas.SetPosition(GuiPosition() + (Offset() * 2));
	float JoystickBackDurationPercentage = !TotalRecordingDuration ? 0.f : 100.f * HoldingJoystickBackDuration / TotalRecordingDuration;
	canvas.DrawString("Pitch Up Amount: " + toPrecision(JoystickBackDurationPercentage, 1) + "%", FontSize(), FontSize());

	if (GuiDrawPitchHistory)
		DrawPitchHistory(canvas);
}

void FastAerialTrainer::DrawBar(
	CanvasWrapper& canvas, std::string text, float value, float maxValue,
	Vector2F barPos, Vector2F barSize,
	LinearColor backgroundColor, std::vector<Range>& colorRanges
)
{
	// Draw background
	canvas.SetPosition(barPos);
	canvas.SetColor(backgroundColor);
	canvas.FillBox(barSize);
	for (Range& range : colorRanges)
	{
		LinearColor preview = *range.color;
		preview.A *= GuiColorPreviewOpacity;
		canvas.SetColor(preview);
		float left = std::min(range.min / maxValue, 1.f);
		float width = std::min((range.max - range.min) / maxValue, 1.f - left);
		canvas.SetPosition(barPos + Vector2F{ left * barSize.X, 0 });
		canvas.FillBox(Vector2F{ width * barSize.X, barSize.Y });
	}

	// Draw colored bar
	canvas.SetPosition(barPos);
	for (Range& range : colorRanges)
	{
		if (range.min <= value && value <= range.max)
		{
			canvas.SetColor(*range.color);
		}
	}
	float result = std::min(1.f, value / maxValue);
	canvas.FillBox(Vector2F{ barSize.X * result, barSize.Y });

	// Draw border
	canvas.SetColor(GuiColorBorder);
	canvas.SetPosition(barPos);
	canvas.DrawBox(barSize);
	for (Range& range : colorRanges)
	{
		canvas.SetColor(GuiColorBorder);
		float result = range.max / maxValue;
		if (result < 1.f)
		{
			canvas.SetPosition(barPos + Vector2{ (int)(result * barSize.X), 0 });
			canvas.FillBox(Vector2F{ 2, barSize.Y });
		}
	}

	// Draw text
	canvas.SetColor(GuiColorBorder);
	canvas.SetPosition(barPos + Vector2F{ 0, barSize.Y });
	canvas.DrawString(text + toPrecision(value, 1) + " ms", FontSize(), FontSize());
}

void FastAerialTrainer::DrawPitchHistory(CanvasWrapper& canvas)
{
	Vector2F offset = (Vector2F() + Offset()) * 2.6f + GuiPosition();
	canvas.SetColor(GuiColorBorder);
	float boxPadding = GuiSize / 200.f;
	canvas.SetPosition(offset - boxPadding);
	canvas.DrawBox(Vector2F{ GuiSize, GuiSize / 10 } + (2 * boxPadding));

	int i = 0;
	int size = (int)InputHistory.size();
	InputHistoryItem prevInput{};
	for (InputHistoryItem currentInput : InputHistory)
	{
		if (i > 0 && currentInput.pitch >= 0 && prevInput.pitch >= 0)
		{
			Vector2F start = { (float)(i - 1) / size * GuiSize, (1 - prevInput.pitch) * GuiSize / 10 };
			Vector2F end = { (float)i / size * GuiSize, (1 - currentInput.pitch) * GuiSize / 10 };
			Vector2F base = start.Y > end.Y ? Vector2F{ end.X, start.Y } : Vector2F{ start.X, end.Y };
			canvas.SetColor(currentInput.boost ? GuiPitchHistoryColorBoost : GuiPitchHistoryColor);
			canvas.FillTriangle(base + offset, start + offset, end + offset); // `FillTriangle` ignores transparency.
			canvas.SetPosition(Vector2F{ start.X, std::max(start.Y, end.Y) } + offset);
			canvas.FillBox(Vector2F{ end.X - start.X, GuiSize / 10 - std::max(start.Y, end.Y) });
		}
		if (currentInput.jumped)
		{
			canvas.SetColor(GuiColorBorder);
			float x = (float)i / size * GuiSize;
			canvas.DrawLine(Vector2F{ x, 0 } + offset, Vector2F{ x, GuiSize / 10.f } + offset, GuiSize / 300.f);
		}
		prevInput = currentInput;
		i++;
	}
}

void FastAerialTrainer::onUnload()
{
}