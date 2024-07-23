#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

struct Range
{
	int min;
	int max;
	LinearColor *color;
};

class FastAerialTrainer : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginSettingsWindow
{
	// Measuring

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

	Vector2 GuiPosition = { 570, 12 };
	int GuiSize = 825;
	Vector2 BarSize() { return { GuiSize, GuiSize / 30 }; }
	Vector2 Offset() { return { 0, GuiSize / 10 }; }
	float FontSize() { return GuiSize / 300.f; }
	int GuiBackgroundOpacity = 150;
	LinearColor GuiColorSuccess = LinearColor(0, 0, 255, 210);
	LinearColor GuiColorWarning = LinearColor(255, 255, 0, 210);
	LinearColor GuiColorFailure = LinearColor(255, 0, 0, 210);
	std::vector<Range> JumpDuration_RangeList =
	{
		Range{ INT_MIN, 180, &GuiColorFailure },
		Range{ 181, 195, &GuiColorWarning },
		Range{ 196, 225, &GuiColorSuccess },
		Range{ 226, 260, &GuiColorWarning },
		Range{ 261, INT_MAX, &GuiColorFailure }
	};
	int JumpDuration_HighestValue = 300;
	std::vector<Range> DoubleJumpDuration_RangeList =
	{
		Range{ INT_MIN, 50, &GuiColorFailure },
		Range{ 51, 70, &GuiColorWarning },
		Range{ 71, 90, &GuiColorSuccess },
		Range{ 91, 110, &GuiColorWarning },
		Range{ 111, INT_MAX, &GuiColorFailure }
	};
	int DoubleJumpDuration_HighestValue = 130;


	// Methods

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
