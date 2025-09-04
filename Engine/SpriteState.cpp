
#include "SpriteState.h"
#include "Graphics.h"
#include "imgui.h"

void SpriteState::Load() {

}

void SpriteState::Init() {

}


void SpriteState::Update() {

}

void SpriteState::Draw() {
	Graphics::clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background
}

void SpriteState::DrawManager() {
	ImGui::Begin("Sprites");
}

void SpriteState::Free() {

}

void SpriteState::Unload() {

}
