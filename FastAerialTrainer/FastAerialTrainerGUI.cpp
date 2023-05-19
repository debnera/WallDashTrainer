#include "pch.h"
#include "FastAerialTrainer.h"

std::string FastAerialTrainer::GetPluginName() {
	return "FastAerialTrainer";
}

void FastAerialTrainer::SetImGuiContext(uintptr_t ctx) {
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

// Render the plugin settings here
// This will show up in bakkesmod when the plugin is loaded at
//  f2 -> plugins -> FastAerialTrainer
void FastAerialTrainer::RenderSettings() 
{
	if (!ac) return;

	ImGui::Text("First Jump Duration Bar :");

	ImGui::NewLine();
	ImGui::PushID(0);

	ImGui::SliderInt("Bar Location X", &JumpDuration_Bar_Pos.X, 0, 1920);
	ImGui::SliderInt("Bar Location Y", &JumpDuration_Bar_Pos.Y, 0, 1080);
	ImGui::SliderInt("Bar Size X", &JumpDuration_Bar_Length, 0, 1000);
	ImGui::SliderInt("Bar Size Y", &JumpDuration_Bar_Height, 0, 100);
	ImGui::SliderInt("Backgroud Bar Opacity", &JumpDuration_BackgroudBar_Opacity, 0, 255);
	ImGui::SliderInt("Value Bar Opacity", &JumpDuration_ValueBar_Opacity, 0, 255);
	ImGui::InputInt("Highest Value", &JumpDuration_HighestValue);

	ImGui::PopID();

	ImGui::NewLine();
	ImGui::Separator();
	ImGui::NewLine();

	ImGui::Text("Time Between First And Double Jump :");

	ImGui::NewLine();
	ImGui::PushID(1);

	ImGui::SliderInt("Bar Location X", &DoubleJumpDuration_Bar_Pos.X, 0, 1920);
	ImGui::SliderInt("Bar Location Y", &DoubleJumpDuration_Bar_Pos.Y, 0, 1080);
	ImGui::SliderInt("Bar Size X", &DoubleJumpDuration_Bar_Length, 0, 1000);
	ImGui::SliderInt("Bar Size Y", &DoubleJumpDuration_Bar_Height, 0, 100);
	ImGui::SliderInt("Backgroud Bar Opacity", &DoubleJumpDuration_BackgroudBar_Opacity, 0, 255);
	ImGui::SliderInt("Value Bar Opacity", &DoubleJumpDuration_ValueBar_Opacity, 0, 255);
	ImGui::InputInt("Highest Value", &DoubleJumpDuration_HighestValue);

	ImGui::PopID();
}

/*
// Do ImGui rendering here
void FastAerialTrainer::Render()
{
	if (!ImGui::Begin(menuTitle_.c_str(), &isWindowOpen_, ImGuiWindowFlags_None))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	ImGui::End();

	if (!isWindowOpen_)
	{
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}
}

// Name of the menu that is used to toggle the window.
std::string FastAerialTrainer::GetMenuName()
{
	return "FastAerialTrainer";
}

// Title to give the menu
std::string FastAerialTrainer::GetMenuTitle()
{
	return menuTitle_;
}

// Don't call this yourself, BM will call this function with a pointer to the current ImGui context
void FastAerialTrainer::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

// Should events such as mouse clicks/key inputs be blocked so they won't reach the game
bool FastAerialTrainer::ShouldBlockInput()
{
	return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}

// Return true if window should be interactive
bool FastAerialTrainer::IsActiveOverlay()
{
	return true;
}

// Called when window is opened
void FastAerialTrainer::OnOpen()
{
	isWindowOpen_ = true;
}

// Called when window is closed
void FastAerialTrainer::OnClose()
{
	isWindowOpen_ = false;
}
*/
