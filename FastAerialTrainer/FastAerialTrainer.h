#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include <chrono>

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

struct Range
{
	Vector2 range;
	int red;
	int green;
	int blue;

	Range(Vector2 _range, int _red, int _green, int _blue) {
		range = _range;
		red = _red;
		green = _green;
		blue = _blue;
	}
};

struct Rec
{
	std::chrono::steady_clock::time_point startTime;
	std::chrono::steady_clock::time_point stopTime;
	int GetDuration() {
		if (stopTime == std::chrono::steady_clock::time_point())
		{
			return 0;
		}
		else
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime).count();
		}
	}
};

class FastAerialTrainer: public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginSettingsWindow/*, public BakkesMod::Plugin::PluginWindow*/
{

	//std::shared_ptr<bool> enabled;


	bool HoldingFirstJump = false;
	std::chrono::steady_clock::time_point holdFirstJumpStartTime;
	std::chrono::steady_clock::time_point holdFirstJumpStopTime;
	int holdFirstJumpDuration;

	std::chrono::steady_clock::time_point DoubleJumpPressedTime;
	int TimeBetweenFirstAndDoubleJump;

	Vector2 JumpDuration_Bar_Pos = { 570, 12 };
	int JumpDuration_Bar_Length = 825;
	int JumpDuration_Bar_Height = 30;
	int JumpDuration_BackgroudBar_Opacity = 150;
	int JumpDuration_ValueBar_Opacity = 210;
	int JumpDuration_HighestValue = 260;
	//std::vector<int> JumpDuration_RangeList = { 140, 180, 220, 260 };
	std::vector<Range> JumpDuration_RangeList = 
	{ 
		Range(Vector2{0, 140}, 255, 0, 0), //red
		Range(Vector2{141, 180}, 255, 255, 0), //yellow
		Range(Vector2{181, 220}, 0, 255, 0), //green
		Range(Vector2{221, 260}, 255, 255, 0) //yellow
	};


	Vector2 DoubleJumpDuration_Bar_Pos = { 570, 86 };
	int DoubleJumpDuration_Bar_Length = 825;
	int DoubleJumpDuration_Bar_Height = 30;
	int DoubleJumpDuration_BackgroudBar_Opacity = 130;
	int DoubleJumpDuration_ValueBar_Opacity = 224;
	int DoubleJumpDuration_HighestValue = 130;
	//std::vector<int> DoubleJumpDuration_RangeList = { 50, 70, 90, 110 };
	std::vector<Range> DoubleJumpDuration_RangeList =
	{
		Range(Vector2{0, 50}, 255, 0, 0), //red
		Range(Vector2{51, 70}, 255, 255, 0), //yellow
		Range(Vector2{71, 90}, 0, 255, 0), //green
		Range(Vector2{91, 110}, 255, 255, 0) //yellow
	};


	bool checkHoldingJoystickBack = false;
	bool wasHoldingJoystickBack = false;
	float holdJoystickBackThreshold = 0.1;
	std::chrono::steady_clock::time_point holdJoystickBackStartTime;
	std::chrono::steady_clock::time_point holdJoystickBackStopTime;
	int HoldingJoystickBackDuration;
	std::vector<Rec> JoystickBackDurations;


	int totalJumpTime;


	void DrawBar(CanvasWrapper canvas, std::string text, int& value, Vector2 barPos, int sizeX, int sizeY, int backgroudBarOpacity, int valueBarOpacity, int highestValue, std::vector<Range>& rangeList);

	void OnTick();
	void RenderCanvas(CanvasWrapper canvas);

	//Boilerplate
	virtual void onLoad();
	virtual void onUnload();

	// Inherited via PluginSettingsWindow
	void RenderSettings() override;
	std::string GetPluginName() override;
	void SetImGuiContext(uintptr_t ctx) override;
	

	// Inherited via PluginWindow
	/*

	bool isWindowOpen_ = false;
	bool isMinimized_ = false;
	std::string menuTitle_ = "FastAerialTrainer";

	virtual void Render() override;
	virtual std::string GetMenuName() override;
	virtual std::string GetMenuTitle() override;
	virtual void SetImGuiContext(uintptr_t ctx) override;
	virtual bool ShouldBlockInput() override;
	virtual bool IsActiveOverlay() override;
	virtual void OnOpen() override;
	virtual void OnClose() override;
	
	*/
};

