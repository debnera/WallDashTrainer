#include "pch.h"
#include "FastAerialTrainer.h"

std::string FastAerialTrainer::GetPluginName() {
	return "FastAerialTrainer";
}

void FastAerialTrainer::SetImGuiContext(uintptr_t ctx) {
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

static void ColorPicker(const char* label, LinearColor *color) {
	LinearColor col = (*color) / 255;
	ImGui::ColorEdit4(label, &col.R, ImGuiColorEditFlags_AlphaBar);
	*color = col * 255;
}

// Render the plugin settings here
// This will show up in bakkesmod when the plugin is loaded at
//  f2 -> plugins -> FastAerialTrainer
void FastAerialTrainer::RenderSettings() 
{
	ImGui::DragInt("GUI Position X", &GuiPosition.X);
	ImGui::DragInt("GUI Position Y", &GuiPosition.Y);
	ImGui::DragInt("GUI Size", &GuiSize);
	ImGui::SliderInt("Background Opacity", &GuiBackgroundOpacity, 0, 255);
	ColorPicker("Success Color", &GuiColorSuccess);
	ColorPicker("Warning Color", &GuiColorWarning);
	ColorPicker("Failure Color", &GuiColorFailure);
	ImGui::DragInt("First Jump Hold - Highest Value", &JumpDuration_HighestValue, 1.f, 1, INT_MAX);
	ImGui::DragInt("Time to Double Jump - Highest Value", &DoubleJumpDuration_HighestValue, 1.f, 1, INT_MAX);
}
