#include "pch.h"
#include "FastAerialTrainer.h"

static bool ColorPicker(const char* label, LinearColor& color) {
	LinearColor col = color / 255;
	bool retVal = ImGui::ColorEdit4(label, &col.R, ImGuiColorEditFlags_AlphaBar);
	color = col * 255;
	return retVal;
}

static bool ColorPickerWithoutAlpha(const char* label, LinearColor& color) {
	LinearColor col = color / 255;
	bool retVal = ImGui::ColorEdit3(label, &col.R);
	color = col * 255;
	return retVal;
}

static bool PercentageSlider(const char* label, float& value, float max = 1.f) {
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

	if (ImGui::SliderFloat("GUI Size", &GuiSize, 0, ScreenSize.X, "%.0f"))
		cvarManager->getCvar(GUI_SIZE).setValue(GuiSize);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("For clearest text use multiples of 350.");

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

	if (ImGui::DragInt("First Jump Hold - Highest Value", &JumpDuration_HighestValue, 1.f, 1, INT_MAX, "%d ms"))
		cvarManager->getCvar(GUI_JUMP_MAX).setValue(JumpDuration_HighestValue);

	if (ImGui::DragInt("Time to Double Jump - Highest Value", &DoubleJumpDuration_HighestValue, 1.f, 1, INT_MAX, "%d ms"))
		cvarManager->getCvar(GUI_DOUBLE_JUMP_MAX).setValue(DoubleJumpDuration_HighestValue);

	if (ImGui::Checkbox("Draw Pitch History", &GuiDrawPitchHistory))
		cvarManager->getCvar(GUI_DRAW_PITCH_HISTORY).setValue(GuiDrawPitchHistory);

	if (ImGui::Checkbox("Draw Boost History", &GuiDrawBoostHistory))
		cvarManager->getCvar(GUI_DRAW_BOOST_HISTORY).setValue(GuiDrawBoostHistory);

	if (ColorPickerWithoutAlpha("Pitch/Boost History Color", GuiPitchHistoryColor))
		cvarManager->getCvar(GUI_COLOR_HISTORY).setValue(GuiPitchHistoryColor);

	if (ImGui::Checkbox("Show First Input Warning in Custom Training", &GuiShowFirstInputWarning))
		cvarManager->getCvar(GUI_SHOW_FIRST_INPUT_WARNING).setValue(GuiShowFirstInputWarning);
}
