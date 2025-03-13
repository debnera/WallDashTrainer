#include "pch.h"
#include "FastAerialTrainer.h"

static bool ColorPicker(const char* label, LinearColor& color)
{
	LinearColor col = color / 255;
	bool retVal = ImGui::ColorEdit4(label, &col.R, ImGuiColorEditFlags_AlphaBar);
	color = col * 255;
	return retVal;
}

static bool ColorPickerWithoutAlpha(const char* label, LinearColor& color)
{
	LinearColor col = color / 255;
	bool retVal = ImGui::ColorEdit3(label, &col.R);
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

void FastAerialTrainer::RenderSettings()
{
	if (ImGui::Checkbox("Enable Plugin", &PluginEnabled))
		cvarManager->getCvar(PLUGIN_ENABLED).setValue(PluginEnabled);

	if (ImGui::DragFloat("Record After Double Jump", &RecordingAfterDoubleJump, 0.005f, 0, FLT_MAX, "%.1f seconds"))
		cvarManager->getCvar(RECORD_AFTER_DOUBLE_JUMP).setValue(RecordingAfterDoubleJump);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Sets how long after the double jump the input recording should stop.");

	if (PercentageSlider("GUI Position X", GuiPositionRelative.X))
		cvarManager->getCvar(GUI_POSITION_RELATIVE_X).setValue(GuiPositionRelative.X);

	if (PercentageSlider("GUI Position Y", GuiPositionRelative.Y))
		cvarManager->getCvar(GUI_POSITION_RELATIVE_Y).setValue(GuiPositionRelative.Y);

	if (ImGui::SliderFloat("GUI Size", &GuiSize, 0, ScreenSize.X, "%.0f pixels"))
		cvarManager->getCvar(GUI_SIZE).setValue(GuiSize);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("For clearest text use multiples of 350 pixels.");

	if (ColorPicker("Border and Text Color", GuiColorBorder))
		cvarManager->getCvar(GUI_BORDER_COLOR).setValue(GuiColorBorder);

	if (ColorPicker("Background Color", GuiColorBackground))
		cvarManager->getCvar(GUI_BACKGROUND_COLOR).setValue(GuiColorBackground);

	if (PercentageSlider("Color Preview Opacity", GuiColorPreviewOpacity, 0.5))
		cvarManager->getCvar(GUI_PREVIEW_OPACTIY).setValue(GuiColorPreviewOpacity);

	if (ColorPicker("Success Color", GuiColorSuccess))
		cvarManager->getCvar(GUI_COLOR_SUCCESS).setValue(GuiColorSuccess);

	if (ColorPicker("Warning Color", GuiColorWarning))
		cvarManager->getCvar(GUI_COLOR_WARNING).setValue(GuiColorWarning);

	if (ColorPicker("Failure Color", GuiColorFailure))
		cvarManager->getCvar(GUI_COLOR_FAILURE).setValue(GuiColorFailure);

	float speed = 0.1f;
	const char* format = "%.1f ms";
	float width = 150;
	float spacing = 20;

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Text("First Jump Timing");
	ImGui::Spacing();
	{
		auto& ranges = JumpDurationRanges;
		auto& cvar = GUI_JUMP_RANGES;
		bool changed = false;
		auto Input = [&](const char* label, float* value, float min, float max)
			{
				if (ImGui::DragFloat(label, value, speed, min, max, format))
					changed = true;
			};

		ImGui::PushID("FirstJumpTiming");
		ImGui::PushItemWidth(width);

		ImGui::BeginGroup();
		Input("Success Low", &ranges[1].max, ranges[0].max, ranges[2].max);
		Input("Success High", &ranges[2].max, ranges[1].max, ranges[3].max);
		ImGui::EndGroup(); ImGui::SameLine(0, spacing); ImGui::BeginGroup();
		Input("Warning Low", &ranges[0].max, ranges[0].min, ranges[1].max);
		Input("Warning High", &ranges[3].max, ranges[2].max, ranges[4].max);
		ImGui::EndGroup(); ImGui::SameLine(0, spacing); ImGui::BeginGroup();
		Input("Failure Low", &ranges[0].min, 0, ranges[0].max);
		Input("Failure High", &ranges[4].max, ranges[3].max, FLT_MAX);
		ImGui::EndGroup();

		ImGui::Spacing();
		if (ImGui::Button("Reset to default"))
			cvarManager->getCvar(cvar).ResetToDefault();

		ImGui::PopItemWidth();
		ImGui::PopID();

		if (changed)
			cvarManager->getCvar(cvar).setValue(RangesToString(ranges));
	}
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Text("Double Jump Timing");
	ImGui::Spacing();
	{
		auto& ranges = DoubleJumpDurationRanges;
		auto& cvar = GUI_DOUBLE_JUMP_RANGES;
		bool changed = false;
		auto Input = [&](const char* label, float* value, float min, float max)
			{
				if (ImGui::DragFloat(label, value, speed, min, max, format))
					changed = true;
			};

		ImGui::PushID("DoubleJumpTiming");
		ImGui::PushItemWidth(width);

		ImGui::BeginGroup();
		Input("Success Low", &ranges[0].min, 0, ranges[0].max);
		Input("Success High", &ranges[0].max, ranges[0].min, ranges[1].max);
		ImGui::EndGroup(); ImGui::SameLine(0, spacing);	ImGui::BeginGroup();
		Input("Warning High", &ranges[1].max, ranges[0].max, ranges[2].max);
		ImGui::EndGroup(); ImGui::SameLine(0, spacing); ImGui::BeginGroup();
		Input("Failure High", &ranges[2].max, ranges[1].max, FLT_MAX);
		ImGui::EndGroup();

		ImGui::Spacing();
		if (ImGui::Button("Reset to default"))
			cvarManager->getCvar(cvar).ResetToDefault();

		ImGui::PopItemWidth();
		ImGui::PopID();

		if (changed)
			cvarManager->getCvar(cvar).setValue(RangesToString(ranges));
	}
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Checkbox("Draw Pitch History", &GuiDrawPitchHistory))
		cvarManager->getCvar(GUI_DRAW_PITCH_HISTORY).setValue(GuiDrawPitchHistory);

	if (ImGui::Checkbox("Draw Boost History", &GuiDrawBoostHistory))
		cvarManager->getCvar(GUI_DRAW_BOOST_HISTORY).setValue(GuiDrawBoostHistory);

	if (ColorPickerWithoutAlpha("Pitch/Boost History Color", GuiPitchHistoryColor))
		cvarManager->getCvar(GUI_COLOR_HISTORY).setValue(GuiPitchHistoryColor);

	if (ImGui::Checkbox("Show First Input Warning in Custom Training", &GuiShowFirstInputWarning))
		cvarManager->getCvar(GUI_SHOW_FIRST_INPUT_WARNING).setValue(GuiShowFirstInputWarning);
}
