#include "pch.h"
#include "WallDashTrainer.h"

static bool ColorPicker(const char* label, LinearColor& color)
{
	LinearColor col = color / 255;
	bool retVal = ImGui::ColorEdit4(label, &col.R, ImGuiColorEditFlags_AlphaBar);
	color = col * 255;
	return retVal;
}

static bool PercentageSlider(const char* label, float& value, float max = 1.f)
{
	float x = value * 100;
	bool retVal = ImGui::SliderFloat(label, &x, 0, 100 * max, "%.1f %%");
	value = x / 100;
	return retVal;
}

static void SpacedSeparator()
{
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
}

void WallDashTrainer::RenderSettings()
{
	if (ImGui::Checkbox("Enable Plugin", &PluginEnabled))
		cvarManager->getCvar(PLUGIN_ENABLED).setValue(PluginEnabled);

	if (ImGui::Checkbox("Show First Jump Timing", &GuiShowFirstJump))
		cvarManager->getCvar(GUI_SHOW_FIRST_JUMP).setValue(GuiShowFirstJump);

	if (ImGui::Checkbox("Show Double Jump Timing", &GuiShowDoubleJump))
		cvarManager->getCvar(GUI_SHOW_DOUBLE_JUMP).setValue(GuiShowDoubleJump);

	SpacedSeparator();

	ImGui::PushID("FirstJump");
	ImGui::Text("First Jump Timing");
	ImGui::Spacing();
	RenderRangePicker(
		JumpDurationRanges,
		GUI_JUMP_RANGES,
		{
			"Success Low",
			"Success High",
			"Warning High",
			"Failure High"
		}
	);
	ImGui::PopID();

	SpacedSeparator();

	ImGui::PushID("DoubleJump");
	ImGui::Text("Double Jump Timing");
	ImGui::Spacing();
	RenderRangePicker(
		DoubleJumpDurationRanges,
		GUI_DOUBLE_JUMP_RANGES,
		{
			"Success Low",
			"Success High",
			"Warning High",
			"Failure High"
		}
	);
	ImGui::PopID();

	SpacedSeparator();

	if (PercentageSlider("GUI Position X", GuiPositionRelative.X))
		cvarManager->getCvar(GUI_POSITION_RELATIVE_X).setValue(GuiPositionRelative.X);

	if (PercentageSlider("GUI Position Y", GuiPositionRelative.Y))
		cvarManager->getCvar(GUI_POSITION_RELATIVE_Y).setValue(GuiPositionRelative.Y);

	if (ImGui::SliderFloat("GUI Size", &GuiSize, 0, ScreenSize.X, "%.0f pixels"))
		cvarManager->getCvar(GUI_SIZE).setValue(GuiSize);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("For clearest text use multiples of 350 pixels.");
	if (ImGui::Button("Reset position and size to default"))
	{
		ResetGUIPositionCVars();
	}

	SpacedSeparator();

	if (ColorPicker("Border and Text Color", GuiColorBorder))
		cvarManager->getCvar(GUI_BORDER_COLOR).setValue(GuiColorBorder);

	if (ColorPicker("Bar Background Color", GuiColorBackground))
		cvarManager->getCvar(GUI_BACKGROUND_COLOR).setValue(GuiColorBackground);

	if (ColorPicker("Backdrop Color", GuiColorBackdrop))
		cvarManager->getCvar(GUI_BACKDROP_COLOR).setValue(GuiColorBackdrop);

	if (PercentageSlider("Color Preview Opacity", GuiColorPreviewOpacity, 0.5))
		cvarManager->getCvar(GUI_PREVIEW_OPACTIY).setValue(GuiColorPreviewOpacity);

	if (ColorPicker("Success Color", GuiColorSuccess))
		cvarManager->getCvar(GUI_COLOR_SUCCESS).setValue(GuiColorSuccess);

	if (ColorPicker("Warning Color", GuiColorWarning))
		cvarManager->getCvar(GUI_COLOR_WARNING).setValue(GuiColorWarning);

	if (ColorPicker("Failure Color", GuiColorFailure))
		cvarManager->getCvar(GUI_COLOR_FAILURE).setValue(GuiColorFailure);
	if (ImGui::Button("Reset colors to default"))
	{
		ResetColorCVars();
	}
}

void WallDashTrainer::ResetGUIPositionCVars()
{
	std::vector<const char*> cvars = {
		GUI_SIZE,
		GUI_POSITION_RELATIVE_X,
		GUI_POSITION_RELATIVE_Y,
	};
    
	for (const auto& cvar : cvars)
	{
		cvarManager->getCvar(cvar).ResetToDefault();
	}
}

void WallDashTrainer::ResetColorCVars()
{
	std::vector<const char*> cvars = {
		GUI_BORDER_COLOR,
		GUI_BACKGROUND_COLOR,
		GUI_BACKDROP_COLOR,
		GUI_COLOR_SUCCESS,
		GUI_COLOR_WARNING,
		GUI_COLOR_FAILURE
	};
    
	for (const auto& cvar : cvars)
	{
		cvarManager->getCvar(cvar).ResetToDefault();
	}
}

void WallDashTrainer::RenderRangePicker(RangeList& rangeList, const char* cvar, std::vector<const char*> labels)
{
	float speed = 0.1f;
	const char* format = "%.1f ms";
	float width = 200;
	float spacing = 20;

	auto values = rangeList.GetValues();

	ImGui::PushItemWidth(width);

	for (int i = 0; i < labels.size(); i++)
	{
		float value = values[i];
		float min = i - 1 < 0 ? 0 : values[i - 1];
		float max = i + 1 >= values.size() ? FLT_MAX : values[i + 1];

		if (ImGui::DragFloat(labels[i], &value, speed, min, max, format))
		{
			rangeList.UpdateValue(i, value);
			cvarManager->getCvar(cvar).setValue(rangeList.ValuesToString());
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Drag or ctrl-click to edit");
	}

	ImGui::Spacing();
	if (ImGui::Button("Reset to default"))
		cvarManager->getCvar(cvar).ResetToDefault();

	ImGui::PopItemWidth();
}