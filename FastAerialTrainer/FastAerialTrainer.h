#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

struct Range
{
	Vector2 range;
	LinearColor color;
};

class FastAerialTrainer : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginSettingsWindow
{
	bool HoldingFirstJump = false;
	float holdFirstJumpStartTime;
	float holdFirstJumpStopTime;
	float holdFirstJumpDuration;

	float DoubleJumpPressedTime;
	float TimeBetweenFirstAndDoubleJump;

	bool checkHoldingJoystickBack = false;
	bool wasHoldingJoystickBack = false;
	float holdJoystickBackThreshold = 0.1; // TODO: Check how much leaning back was done.
	float holdJoystickBackStartTime;
	float HoldingJoystickBackDuration;

	float totalJumpTime;


	// Styling

	Vector2 JumpDuration_Bar_Pos = { 570, 12 };
	int JumpDuration_Bar_Length = 825;
	int JumpDuration_Bar_Height = 30;
	int JumpDuration_BackgroudBar_Opacity = 150;
	int JumpDuration_ValueBar_Opacity = 210;
	int JumpDuration_HighestValue = 300;
	std::vector<Range> JumpDuration_RangeList =
	{
		Range{ Vector2{ 0, 180 }, LinearColor(255, 0, 0, 210) },
		Range{ Vector2{ 181, 195 }, LinearColor(255, 255, 0, 210) },
		Range{ Vector2{ 196, 225 }, LinearColor(0, 0, 255, 210) },
		Range{ Vector2{ 226, 260 }, LinearColor(255, 255, 0, 210) }
	};


	Vector2 DoubleJumpDuration_Bar_Pos = { 570, 86 };
	int DoubleJumpDuration_Bar_Length = 825;
	int DoubleJumpDuration_Bar_Height = 30;
	int DoubleJumpDuration_BackgroudBar_Opacity = 130;
	int DoubleJumpDuration_ValueBar_Opacity = 224;
	int DoubleJumpDuration_HighestValue = 130;
	std::vector<Range> DoubleJumpDuration_RangeList =
	{
		Range{ Vector2{ 0, 50 }, LinearColor(255, 0, 0 , 224) },
		Range{ Vector2{ 51, 70 }, LinearColor(255, 255, 0, 224) },
		Range{ Vector2{ 71, 90 }, LinearColor(0, 0, 255, 224) },
		Range{ Vector2{ 91, 110 }, LinearColor(255, 255, 0, 224) }
	};

	float GetCurrentTime();
	void OnTick(CarWrapper car);

	void DrawBar(CanvasWrapper canvas, std::string text, float value, float maxValue, Vector2 barPos, Vector2 barSize, int backgroudBarOpacity, std::vector<Range>& colorRanges);
	void RenderCanvas(CanvasWrapper canvas);

	virtual void onLoad();
	virtual void onUnload();

	void RenderSettings() override;
	std::string GetPluginName() override;
	void SetImGuiContext(uintptr_t ctx) override;
};

