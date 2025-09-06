
#include "SpriteState.h"
#include "Graphics.h"
#include "imgui.h"
#include "InputManager.h"
#include "iostream" //for std
#include <GLFW/glfw3.h>

using namespace Engine;

std::vector<Sprite> SpriteState::sprites = {};
bool SpriteState::dropdown = true;
bool SpriteState::edit = true;
bool SpriteState::dragndrop = false;

void SpriteState::Load() {

}

void SpriteState::Init() {

}


void SpriteState::Update() {
	if (Engine::Input::IsKeyPressed(GLFW_KEY_E))
	{
		std::cout << "<3 WE WORKING KING <3\n";
	}
}

void SpriteState::Draw() {
	Graphics::clearScreen(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background
}

void SpriteState::DrawManager() {

	if (!dropdown) return;

	if (ImGui::Begin("Sprites", &dropdown)) {
		std::cout << "<3 WE WORKING KING <3\n";
		ImGui::Separator();
	}
	ImGui::End();
}

void SpriteState::Free() {

}

void SpriteState::Unload() {

}
