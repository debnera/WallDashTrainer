#include "pch.h"
#include "FastAerialTrainer.h"
#include "bakkesmod/wrappers/Engine/WorldInfoWrapper.h"


BAKKESMOD_PLUGIN(FastAerialTrainer, "FastAerialTrainer", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

float FastAerialTrainer::GetCurrentTime() {
	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (!server) return 0;
	WorldInfoWrapper world = server.GetWorldInfo();
	if (!world) return 0;
	return world.GetTimeSeconds();
}

void FastAerialTrainer::onLoad()
{
	_globalCvarManager = cvarManager;

	gameWrapper->RegisterDrawable(std::bind(&FastAerialTrainer::RenderCanvas, this, std::placeholders::_1));

	gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.SetVehicleInput", std::bind(&FastAerialTrainer::OnTick, this, std::placeholders::_1));

	//when car uses first jump
	gameWrapper->HookEventWithCaller<CarWrapper>("Function CarComponent_Jump_TA.Active.BeginState",
		[this](CarWrapper car, void* params, std::string eventname) {

			float now = GetCurrentTime();

			holdFirstJumpStartTime = now;
			HoldingFirstJump = true;

			HoldingJoystickBackDuration = 0;
			checkHoldingJoystickBack = true;
			wasHoldingJoystickBack = false;

			ControllerInput inputs = car.GetInput();
			if (inputs.Pitch > holdJoystickBackThreshold)
			{
				holdJoystickBackStartTime = holdFirstJumpStartTime;
			}
		});

	gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.OnJumpReleased",
		[this](CarWrapper car, void* params, std::string eventname) {

			float now = GetCurrentTime();

			if (HoldingFirstJump)
			{
				HoldingFirstJump = false;
				holdFirstJumpStopTime = now;
			}
		});

	//when car uses double jump
	gameWrapper->HookEventWithCaller<CarWrapper>("Function CarComponent_DoubleJump_TA.Active.BeginState",
		[this](CarWrapper caller, void* params, std::string eventname) {

			float now = GetCurrentTime();

			DoubleJumpPressedTime = now;
			TimeBetweenFirstAndDoubleJump = DoubleJumpPressedTime - holdFirstJumpStopTime;

			checkHoldingJoystickBack = false;
			if (wasHoldingJoystickBack)
			{
				HoldingJoystickBackDuration += now - holdJoystickBackStartTime;
			}

			totalJumpTime = DoubleJumpPressedTime - holdFirstJumpStartTime;
		});
}

void FastAerialTrainer::OnTick(CarWrapper car)
{
	float now = GetCurrentTime();

	if (HoldingFirstJump)
	{
		holdFirstJumpDuration = now - holdFirstJumpStartTime;
	}
	else
	{
		holdFirstJumpDuration = holdFirstJumpStopTime - holdFirstJumpStartTime;
	}

	ControllerInput inputs = car.GetInput();
	if (checkHoldingJoystickBack)
	{
		if (inputs.Pitch >= holdJoystickBackThreshold)
		{
			if (!wasHoldingJoystickBack)
			{
				holdJoystickBackStartTime = now;
				wasHoldingJoystickBack = true;
			}
		}
		else
		{
			if (wasHoldingJoystickBack)
			{
				wasHoldingJoystickBack = false;
				HoldingJoystickBackDuration += now - holdJoystickBackStartTime;
			}
		}
	}
}

void FastAerialTrainer::RenderCanvas(CanvasWrapper canvas)
{
	DrawBar(canvas, "Hold First Jump in ms: ", holdFirstJumpDuration * 1000, JumpDuration_HighestValue,
		JumpDuration_Bar_Pos, Vector2{ JumpDuration_Bar_Length, JumpDuration_Bar_Height },
		JumpDuration_BackgroudBar_Opacity, JumpDuration_RangeList);

	DrawBar(canvas, "Time to Double Jump in ms: ", TimeBetweenFirstAndDoubleJump * 1000, DoubleJumpDuration_HighestValue,
		DoubleJumpDuration_Bar_Pos, Vector2{ DoubleJumpDuration_Bar_Length, DoubleJumpDuration_Bar_Height },
		DoubleJumpDuration_BackgroudBar_Opacity, DoubleJumpDuration_RangeList);

	canvas.SetPosition(Vector2{ 570, 185 });
	float JoystickBackDurationPercentage = !totalJumpTime ? 0.f : 100.f * HoldingJoystickBackDuration / totalJumpTime;
	canvas.DrawString("Tilt between jumps: " + std::to_string(JoystickBackDurationPercentage) + "%", 2.5f, 2.5f);
}

void FastAerialTrainer::DrawBar(CanvasWrapper canvas, std::string text, float value, float maxValue, Vector2 barPos, Vector2 barSize, int backgroudBarOpacity, std::vector<Range>& colorRanges)
{
	// Draw background
	canvas.SetPosition(barPos);
	canvas.SetColor(255, 255, 255, backgroudBarOpacity);
	canvas.FillBox(barSize);

	// Draw colored bar
	canvas.SetPosition(barPos);
	canvas.SetColor(colorRanges.front().color);
	for (Range& range : colorRanges)
	{
		if (range.range.X <= value && value <= range.range.Y)
		{
			canvas.SetColor(range.color);
		}
	}
	float result = std::min(1.f, value / maxValue);
	int barLength = barSize.X * result;
	canvas.FillBox(Vector2{ barLength, barSize.Y });

	// Draw border
	canvas.SetColor(255, 255, 255, 255);
	canvas.SetPosition(barPos);
	canvas.DrawBox(barSize);

	for (Range& range : colorRanges)
	{
		canvas.SetColor(255, 255, 255, 255);
		float result = float(range.range.Y) / float(maxValue);
		int barLength = barSize.X * result;
		canvas.SetPosition(Vector2{ barPos.X + barLength, barPos.Y });
		canvas.FillBox(Vector2{ 2, barSize.Y });
	}

	// Draw text
	canvas.SetColor(255, 255, 255, 255);
	canvas.SetPosition(Vector2{ barPos.X, barPos.Y + 31 });
	canvas.DrawString(text + std::to_string(value), 2.5f, 2.5f);
}

void FastAerialTrainer::onUnload()
{
}