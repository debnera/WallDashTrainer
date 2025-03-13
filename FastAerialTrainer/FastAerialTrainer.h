#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "PersistentStorage.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

constexpr auto PLUGIN_ENABLED = "fast_aerial_trainer_enabled";
constexpr auto RECORD_AFTER_DOUBLE_JUMP = "fast_aerial_trainer_record_after_double_jump";
constexpr auto GUI_POSITION_RELATIVE_X = "fast_aerial_trainer_gui_pos_rel_x";
constexpr auto GUI_POSITION_RELATIVE_Y = "fast_aerial_trainer_gui_pos_rel_y";
constexpr auto GUI_SIZE = "fast_aerial_trainer_gui_size";
constexpr auto GUI_BORDER_COLOR = "fast_aerial_trainer_gui_border_color";
constexpr auto GUI_BACKGROUND_COLOR = "fast_aerial_trainer_gui_background_color";
constexpr auto GUI_PREVIEW_OPACTIY = "fast_aerial_trainer_gui_preview_opactiy";
constexpr auto GUI_COLOR_SUCCESS = "fast_aerial_trainer_gui_color_success";
constexpr auto GUI_COLOR_WARNING = "fast_aerial_trainer_gui_color_warning";
constexpr auto GUI_COLOR_FAILURE = "fast_aerial_trainer_gui_color_failure";
constexpr auto GUI_JUMP_MAX = "fast_aerial_trainer_gui_jump_max";
constexpr auto GUI_DOUBLE_JUMP_MAX = "fast_aerial_trainer_gui_double_jump_max";
constexpr auto GUI_DRAW_HISTORY = "fast_aerial_trainer_gui_draw_history";
constexpr auto GUI_COLOR_HISTORY = "fast_aerial_trainer_gui_color_history";

struct Range
{
	int min;
	int max;
	LinearColor* color;
};

struct InputHistoryItem
{
	float pitch;
	bool boost;
	bool jumped;
};

class FastAerialTrainer : public BakkesMod::Plugin::BakkesModPlugin, public SettingsWindowBase
{
	std::shared_ptr<PersistentStorage> persistentStorage;


	// Settings

	bool PluginEnabled = true;
	bool IsInReplay = false;
	float RecordingAfterDoubleJump = 0.2;


	// Measuring

	bool HoldingFirstJump = false;
	float HoldFirstJumpStartTime = 0;
	float HoldFirstJumpStopTime = 0;
	float HoldFirstJumpDuration = 0;


	bool DoubleJumpPossible = false;
	float DoubleJumpPressedTime = 0;
	float TimeBetweenFirstAndDoubleJump = 0;
	float HoldingJoystickBackDuration = 0;
	float TotalRecordingDuration = 0;

	float LastTickTime = 0;
	std::vector<InputHistoryItem> InputHistory;


	// Styling

	Vector2F GuiPositionRelative = { 0.5, 0.01 };
	Vector2 ScreenSize = { 1920, 1080 };
	float GuiSize = 700;
	Vector2F BarSize() { return { GuiSize, GuiSize / 24.f }; }
	Vector2F Offset() { return { 0, GuiSize / 10.f }; }
	float FontSize() { return GuiSize / 350.f; }
	Vector2F GuiPosition()
	{
		return {
			std::lerp(0.f, ScreenSize.X - GuiSize, GuiPositionRelative.X),
			std::lerp(0.f, ScreenSize.Y - 0.25f * GuiSize, GuiPositionRelative.Y)
		};
	}
	float GuiColorPreviewOpacity = 0.2f;
	LinearColor GuiColorBorder = LinearColor(255, 255, 255, 255);
	LinearColor GuiColorBackground = LinearColor(255, 255, 255, 150);
	LinearColor GuiColorSuccess = LinearColor(0, 0, 255, 210);
	LinearColor GuiColorWarning = LinearColor(255, 255, 0, 210);
	LinearColor GuiColorFailure = LinearColor(255, 0, 0, 210);
	std::vector<Range> JumpDuration_RangeList =
	{
		Range{ 0, 180, &GuiColorFailure },
		Range{ 180, 195, &GuiColorWarning },
		Range{ 195, 225, &GuiColorSuccess },
		Range{ 225, 260, &GuiColorWarning },
		Range{ 260, INT_MAX, &GuiColorFailure }
	};
	int JumpDuration_HighestValue = 300;
	std::vector<Range> DoubleJumpDuration_RangeList =
	{
		Range{ 0, 75, &GuiColorSuccess },
		Range{ 75, 110, &GuiColorWarning },
		Range{ 110, INT_MAX, &GuiColorFailure }
	};
	int DoubleJumpDuration_HighestValue = 130;
	LinearColor GuiPitchHistoryColor = LinearColor(240, 240, 240, 255);
	bool GuiDrawPitchHistory = true;


	// Methods

	bool IsActive();

	float GetCurrentTime();
	void OnTick(CarWrapper car);

	void RenderCanvas(CanvasWrapper canvas);
	void DrawBar(CanvasWrapper& canvas, std::string text, float value, float maxValue, Vector2F barPos, Vector2F barSize, LinearColor backgroundColor, std::vector<Range>& colorRanges);
	void DrawPitchHistory(CanvasWrapper& canvas);
	void DrawBoostHistory(CanvasWrapper& canvas);

	virtual void onLoad();
	virtual void onUnload();

	void RenderSettings() override;
};
