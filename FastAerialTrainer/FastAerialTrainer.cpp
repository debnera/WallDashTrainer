#include "pch.h"
#include "FastAerialTrainer.h"


BAKKESMOD_PLUGIN(FastAerialTrainer, "FastAerialTrainer", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void FastAerialTrainer::onLoad()
{
	_globalCvarManager = cvarManager;


	gameWrapper->RegisterDrawable(std::bind(&FastAerialTrainer::RenderCanvas, this, std::placeholders::_1));

	/*cvarManager->registerNotifier("my_aweseome_notifier", [&](std::vector<std::string> args) {
		cvarManager->log("Hello notifier!");
	}, "", 0);*/


	gameWrapper->HookEvent("Function TAGame.Car_TA.SetVehicleInput", std::bind(&FastAerialTrainer::OnTick, this));

	//Function CarComponent_Jump_TA.Active.BeginState //when car uses first jump
	//Function CarComponent_DoubleJump_TA.Active.BeginState //when car uses double jump

	gameWrapper->HookEventWithCaller<CarWrapper>("Function CarComponent_Jump_TA.Active.BeginState",
		[this](CarWrapper caller, void* params, std::string eventname) {
			

			holdFirstJumpStartTime = std::chrono::steady_clock::now();
			HoldingFirstJump = true;

			JoystickBackDurations.clear();
			checkHoldingJoystickBack = true;
			wasHoldingJoystickBack = false;

			CarWrapper car = gameWrapper->GetLocalCar();
			if (!car)
				return;
			ControllerInput inputs = car.GetInput();
			if (inputs.Pitch > holdJoystickBackThreshold)
			{
				Rec rec;
				rec.startTime = holdFirstJumpStartTime;
				JoystickBackDurations.push_back(rec);
			}

		});

	gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.OnJumpReleased",
		[this](CarWrapper caller, void* params, std::string eventname) {

			if (HoldingFirstJump)
			{
				HoldingFirstJump = false;
				holdFirstJumpStopTime = std::chrono::steady_clock::now();
			}

		});

	gameWrapper->HookEventWithCaller<CarWrapper>("Function CarComponent_DoubleJump_TA.Active.BeginState",
		[this](CarWrapper caller, void* params, std::string eventname) {

			DoubleJumpPressedTime = std::chrono::steady_clock::now();
			TimeBetweenFirstAndDoubleJump = std::chrono::duration_cast<std::chrono::milliseconds>(DoubleJumpPressedTime - holdFirstJumpStopTime).count();


			checkHoldingJoystickBack = false;
			if (JoystickBackDurations.back().stopTime == std::chrono::steady_clock::time_point()) //if stop time doesn't have a value
				JoystickBackDurations.back().stopTime = DoubleJumpPressedTime;

			HoldingJoystickBackDuration = 0;
			for (Rec rec : JoystickBackDurations)
			{
				HoldingJoystickBackDuration += rec.GetDuration();
			}


			totalJumpTime = std::chrono::duration_cast<std::chrono::milliseconds>(DoubleJumpPressedTime - holdFirstJumpStartTime).count();
		});
}

void FastAerialTrainer::OnTick()
{
	CarWrapper car = gameWrapper->GetLocalCar();
	if (!car)
		return;

	if (HoldingFirstJump)
	{
		holdFirstJumpDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - holdFirstJumpStartTime).count();
	}
	else
	{
		holdFirstJumpDuration = std::chrono::duration_cast<std::chrono::milliseconds>(holdFirstJumpStopTime - holdFirstJumpStartTime).count();
	}

	ControllerInput inputs = car.GetInput();
	if (checkHoldingJoystickBack)
	{
		if (inputs.Pitch > holdJoystickBackThreshold)
		{
			if (!wasHoldingJoystickBack)
			{
				/*if (JoystickBackDurations.back().stopTime != std::chrono::steady_clock::time_point())
				{
					Rec rec;
					rec.startTime = std::chrono::steady_clock::now();
					JoystickBackDurations.push_back(rec);
				}*/
				if (JoystickBackDurations.size() == 0)
				{
					Rec rec;
					rec.startTime = std::chrono::steady_clock::now();
					JoystickBackDurations.push_back(rec);
				}
				else if (JoystickBackDurations.back().stopTime != std::chrono::steady_clock::time_point())
				{
					Rec rec;
					rec.startTime = std::chrono::steady_clock::now();
					JoystickBackDurations.push_back(rec);
				}
				wasHoldingJoystickBack = true;
			}
		}
		else
		{
			if (wasHoldingJoystickBack)
			{
				wasHoldingJoystickBack = false;
				JoystickBackDurations.back().stopTime = std::chrono::steady_clock::now();
			}
		}
	}

}

void FastAerialTrainer::RenderCanvas(CanvasWrapper canvas)
{
	CarWrapper car = gameWrapper->GetLocalCar();
	if (!car)
		return;


	ControllerInput inputs = car.GetInput();

	/*canvas.SetColor(255, 255, 255, 255);
	canvas.DrawString("HasFlip : " + std::to_string(car.HasFlip()));
	canvas.SetPosition(Vector2{ 10, 15 });
	canvas.DrawString("GetbDoubleJumped : " + std::to_string(car.GetbDoubleJumped()));
	canvas.SetPosition(Vector2{ 10, 30 });
	canvas.DrawString("GetbJumped : " + std::to_string(car.GetbJumped()));
	canvas.SetPosition(Vector2{ 10, 45 });
	canvas.DrawString("GetbCanJump : " + std::to_string(car.GetbCanJump()));
	canvas.SetPosition(Vector2{ 10, 60 });
	canvas.DrawString("Handbrake : " + std::to_string(inputs.Handbrake));
	canvas.SetPosition(Vector2{ 10, 75 });
	canvas.DrawString("Jump : " + std::to_string(inputs.Jump));
	canvas.SetPosition(Vector2{ 10, 90 });
	canvas.DrawString("Jumped : " + std::to_string(inputs.Jumped));*/



	//canvas.SetPosition(Vector2{ 10, 110 });
	//canvas.DrawString("holdFirstJumpStartTime : " + std::to_string(holdFirstJumpStartTime));
	//canvas.SetPosition(Vector2{ 10, 125 });
	//canvas.DrawString("holdFirstJumpStopTime : " + std::to_string(holdFirstJumpStopTime));
	//canvas.SetPosition(Vector2{ 10, 140 });
	//canvas.DrawString("holdFirstJumpDuration : " + std::to_string(holdFirstJumpDuration));
	////canvas.SetPosition(Vector2{ 10, 155 });
	////canvas.DrawString("DoubleJumpPressedTime : " + std::to_string(DoubleJumpPressedTime));
	//canvas.SetPosition(Vector2{ 10, 170 });
	//canvas.DrawString("TimeBetweenFirstAndDoubleJump : " + std::to_string(TimeBetweenFirstAndDoubleJump));
	//canvas.SetPosition(Vector2{ 10, 185 });
	//canvas.DrawString("totalJumpTime : " + std::to_string(totalJumpTime));
	//canvas.SetPosition(Vector2{ 10, 200 });
	//canvas.DrawString("HoldingJoystickBackDuration : " + std::to_string(HoldingJoystickBackDuration));
	//canvas.SetPosition(Vector2{ 10, 215 });
	//canvas.DrawString("JoystickBackDurations size : " + std::to_string(JoystickBackDurations.size()));

	Vector2 pos = Vector2{ 10, 230 };
	for (int n = 0; n < JoystickBackDurations.size(); n++)
	{
		auto rec = JoystickBackDurations[n];
		int aaa = n * 15;
		canvas.SetPosition(Vector2{ 10, pos.Y + aaa });
		canvas.DrawString(std::to_string(n) + " : " + std::to_string(rec.GetDuration()));
	}


	//holdFirstJumpDuration background bar
	DrawBar(canvas, "holdFirstJumpDuration (ms) : ", holdFirstJumpDuration, JumpDuration_Bar_Pos, JumpDuration_Bar_Length,
		JumpDuration_Bar_Height, JumpDuration_BackgroudBar_Opacity, JumpDuration_ValueBar_Opacity, JumpDuration_HighestValue, JumpDuration_RangeList);


	//TimeBetweenFirstAndDoubleJump background bar
	DrawBar(canvas, "TimeBetweenFirstAndDoubleJump (ms) : ", TimeBetweenFirstAndDoubleJump, DoubleJumpDuration_Bar_Pos, DoubleJumpDuration_Bar_Length,
		DoubleJumpDuration_Bar_Height, DoubleJumpDuration_BackgroudBar_Opacity, DoubleJumpDuration_ValueBar_Opacity, DoubleJumpDuration_HighestValue, DoubleJumpDuration_RangeList);


	canvas.SetPosition(Vector2{ 570, 185 });
	float JoystickBack_DurationPercentage = (float(HoldingJoystickBackDuration) / float(totalJumpTime)) * 100.f;
	canvas.DrawString("JoystickBack_DurationPercentage : " + std::to_string(JoystickBack_DurationPercentage) + "%", 2.5f, 2.5f);
}

void FastAerialTrainer::DrawBar(CanvasWrapper canvas, std::string text, int& value, Vector2 barPos, int sizeX, int sizeY, int backgroudBarOpacity, int valueBarOpacity, int highestValue, std::vector<Range>& rangeList)
{
	canvas.SetPosition(barPos);
	canvas.SetColor(255, 255, 255, backgroudBarOpacity);
	canvas.FillBox(Vector2{ sizeX, sizeY });
	if (holdFirstJumpDuration != 0)
	{
		canvas.SetPosition(barPos);

		canvas.SetColor(255, 0, 0, valueBarOpacity);

		for (Range range : rangeList)
		{
			if (value >= range.range.X && value <= range.range.Y)
			{
				canvas.SetColor(range.red, range.green, range.blue, valueBarOpacity);
			}
		}

		float result = float(value) / float(highestValue);
		if (value >= highestValue)
			result = 1.f;
		//LOG("{}", result);
		int barLength = sizeX * result;
		canvas.FillBox(Vector2{ barLength, sizeY });
	}


	canvas.SetColor(255, 255, 255, 255);
	canvas.SetPosition(Vector2{ barPos.X, barPos.Y + 31 });
	canvas.DrawString(text + std::to_string(value), 2.5f, 2.5f);

	canvas.SetColor(255, 255, 255, 255);
	canvas.SetPosition(barPos);
	canvas.DrawBox(Vector2{ sizeX, sizeY });

	for (Range range : rangeList)
	{
		canvas.SetColor(255, 255, 255, 255);
		float result2 = float(range.range.Y) / float(highestValue);
		int barLength2 = sizeX * result2;
		canvas.SetPosition(Vector2{ barPos.X + barLength2, barPos.Y });
		canvas.FillBox(Vector2{ 2, sizeY });
	}
}

void FastAerialTrainer::onUnload()
{
}