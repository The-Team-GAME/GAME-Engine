#pragma once

#include "StateManager.h"
#include "core.h"
#include "ObjectFactory.h"
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <stack>
#include <iostream> // For debugging output

namespace fs = std::filesystem;

// The file path of the image that was selected
static std::string selected_file_path;

// The directory where the project is located, selected by the user
static std::string root_path; //currently unused

// My file explorer function :>
void FileExplorer(bool* p_open) {

	static fs::path current_path;
	static bool first_run = true;
	static std::stack<fs::path> history;


	if (first_run) {

		first_run = false;
		fs::path home_path;

#ifdef _WIN32

		char* user_profile = nullptr;
		size_t size = 0;

		if (_dupenv_s(&user_profile, &size, "USERPROFILE") == 0 && user_profile != nullptr) {

			home_path = fs::path(user_profile) / "Downloads";
			free(user_profile);
		}

#else

		const char* home_dir = getenv("HOME");

		if (home_dir) {
			home_path = fs::path(home_dir) / "Downloads";
		}

#endif

		if (home_path.empty()) {
			current_path = fs::current_path();
		}

		else {
			current_path = home_path;
		}
	}

	if (ImGui::Begin("File Explorer", p_open)) {

		if (!history.empty() && ImGui::Button("Back")) {
			current_path = history.top();
			history.pop();
		}

		ImGui::SameLine();


		//goes to desktop
		if (ImGui::Button("..")) {

			if (current_path.has_parent_path()) {
				history.push(current_path);
				current_path = current_path.parent_path();
			}
		}

		ImGui::Separator();

		ImGui::Text("Select an image file:");
		ImGui::Text("Current Path: %s", current_path.string().c_str());

		if (!fs::exists(current_path) || !fs::is_directory(current_path)) {

			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Invalid directory path!");
			ImGui::End();
			return;
		}

		// checks pathways if files are .jpg .png .jpeg
		std::vector<fs::directory_entry> entries;

		for (const auto& entry : fs::directory_iterator(current_path)) {

			if (fs::is_directory(entry.path())) {
				entries.push_back(entry);
			}

			else {
				std::string extension = entry.path().extension().string();
				std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

				if (extension == ".jpg" || extension == ".png" || extension == ".jpeg") {
					entries.push_back(entry);
				}
			}
		}



		//sorts pathways
		std::sort(entries.begin(), entries.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {

			bool a_is_dir = fs::is_directory(a.path());
			bool b_is_dir = fs::is_directory(b.path());
			if (a_is_dir && !b_is_dir) { return true; }
			if (!a_is_dir && b_is_dir) { return false; }
			return a.path().filename().string() < b.path().filename().string();
			});

		for (const auto& entry : entries) {

			bool is_directory = fs::is_directory(entry.path());

			const char* icon = is_directory ? "[DIR]" : "[FILE]";

			if (ImGui::Selectable((icon + std::string(" ") + entry.path().filename().string()).c_str(), false, 0, ImVec2(0, 20))) {

				if (is_directory) {
					history.push(current_path);
					current_path /= entry.path().filename();
				}

				else {
					selected_file_path = entry.path().string();
					*p_open = false;
				}
			}
		}
	}
	ImGui::End();
}


struct DroppedImage {
	int textureId;
	ImVec2 position;
};


class SpriteState : public Engine::GameState<SpriteState> {

private:

	int linearMonkeyImageId = -1;
	int nearestMonkeyImageId = -1;
	std::vector<int> loadedImageIds; //vector id for all my new loaded images
	std::vector<DroppedImage> droppedImages; // vector for dropped images

	bool showFileExplorerSource = false;
	bool fileCopied = false;

	struct ImageData {
		unsigned int texture_id;
		const char* name;
	};


	std::vector<ImageData> ImageAspectRatio;

	float spritePosX = 0.0f;

	// Helper function to load all images from the assets directory
	void LoadAllAssets() {

		loadedImageIds.clear();

		fs::path assets_path = fs::current_path() / "Engine" / "Assets" / "images";

		if (!fs::exists(assets_path) || !fs::is_directory(assets_path)) {

			// Create the directory if it doesn't exist
			fs::create_directories(assets_path);

			return;
		}

		for (const auto& entry : fs::directory_iterator(assets_path)) {

			if (entry.is_regular_file()) {

				std::string extension = entry.path().extension().string();
				std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

				if (extension == ".jpg" || extension == ".png" || extension == ".jpeg") {
					int textureId = Engine::GRAPHICS->loadTexture(entry.path().string().c_str(), Graphics::MinifyingFilter::Nearest);
					if (textureId != -1) {
						loadedImageIds.push_back(textureId);
					}
				}
			}
		}
	}




public:

	void Load() {};


	void Init() {

		linearMonkeyImageId = Engine::GRAPHICS->loadTexture("Assets/images/monkey.png");
		nearestMonkeyImageId = Engine::GRAPHICS->loadTexture("Assets/images/monkey.png", Graphics::MinifyingFilter::Nearest);

		// Load all existing images from the assets folder on startup
		LoadAllAssets();

		std::cout << "Current path is " << std::filesystem::current_path() << std::endl;
	};



	void Update() {};


	void Draw() {

		//// --- Main Scene Drop Target ---
		//// Full Screen invisible window to drop images
		//ImGui::SetNextWindowPos(ImVec2(0, 0));
		//ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		//ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0)); // Make the background transparent
		//ImGui::Begin("Main Scene Target", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

		//ImGui::PopStyleColor();

		//if (ImGui::BeginDragDropTarget()) {

		//	if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPRITEDROP_ID")) {

		//		// Retrieve the image ID from the payload
		//		int droppedImageId = *(const int*)payload->Data;
		//		ImVec2 dropPos = ImGui::GetMousePos();

		//		// This is where you call your engine's logic to create a new sprite object.
		//		// Example: Engine::CORE->createSprite(droppedImageId, dropPos.x, dropPos.y);
		//		std::cout << "Dropped image with ID: " << droppedImageId << " at position (" << dropPos.x << ", " << dropPos.y << ")" << std::endl;
		//	}
		//	ImGui::EndDragDropTarget();
		//}
		//ImGui::End();


		// Main scene texture drawing
		Engine::GRAPHICS->clearScreen(0.1f, 0.1f, 0.1f, 1.0f);
		Engine::GRAPHICS->drawTexture(linearMonkeyImageId, 200.0f, 150.0f, 150.0f, 210.0f);
		Engine::GRAPHICS->drawTexture(nearestMonkeyImageId, 400.0f, 150.0f, 150.0f, 210.0f);
		Engine::GRAPHICS->drawTextureRegion(
			nearestMonkeyImageId, 600.0f, 150.0f, 150.0f, 210.0f, spritePosX += Engine::CORE->getDt() * 10.0f, 0.0f, 15.0f, 21.0f
		);



		// --- Image Menu ---
		ImGui::Begin("Image Menu");
		ImGui::Text("Project Root: %s", fs::current_path().string().c_str());


		ImGui::Separator();

		if (ImGui::Button("Browse for Image")) {
			showFileExplorerSource = true;
			fileCopied = false;
			selected_file_path.clear();
		}

		if (showFileExplorerSource) {
			FileExplorer(&showFileExplorerSource);
		}

		if (!showFileExplorerSource && !selected_file_path.empty() && !fileCopied) {

			try {
				fs::path source_path(selected_file_path);
				fs::path destination_dir = fs::current_path() / "Engine" / "Assets" / "images";
				fs::path destination_path = destination_dir / source_path.filename();

				fs::create_directories(destination_dir);
				fs::copy_file(source_path, destination_path, fs::copy_options::overwrite_existing);
				std::cout << "File copied successfully to: " << destination_path << std::endl;

				LoadAllAssets();
				selected_file_path.clear();
				fileCopied = true;

			}

			catch (const fs::filesystem_error& e) {
				fprintf(stderr, "Filesystem error: %s\n", e.what());
			}
		}
		ImGui::End();



		// --- Sprite Dropper ---
		ImGui::Begin("Sprite Dropper");
		ImGui::Text("Drop Images:");

		for (int imageId : loadedImageIds) { //loads all of my images within the file

			ImGui::PushID(imageId);

			// Push a style color change to make the button background transparent
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));


			// Create the transparent button. Its area still captures clicks and drag events.
			ImGui::Button("##invisible_button", ImVec2(50, 50));


			// Pop the style colors to revert the change for other buttons
			ImGui::PopStyleColor(3);

			// Check if the button is being dragged.
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {

				// Set the payload with the image ID
				ImGui::SetDragDropPayload("SPRITEDROP_ID", &imageId, sizeof(int));


				// Draw the drag preview
				ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(imageId)), ImVec2(50, 50));
				ImGui::Text("Dragging Image...");
				ImGui::EndDragDropSource();
			}

			// Placing a button on top of the image
			ImVec2 min = ImGui::GetItemRectMin();
			ImVec2 max = ImGui::GetItemRectMax();

			ImGui::GetWindowDrawList()->AddImage( reinterpret_cast<void*>(static_cast<intptr_t>(imageId)), min, max, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImVec4(1, 1, 1, 1))
			);


			ImGui::PopID(); // Remove the unique ID from the stack
			ImGui::SameLine(); // Places images on a same line


			//if (ImGui::BeginDragDropTarget()) {

			//	if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPRITEDROP_ID")) {
			//		int droppedImageId = *(const int*)payload->Data;
			//		ImVec2 dropPos = ImGui::GetMousePos();

			//		// Create a new DroppedImage instance and add it to the vector
			//		droppedImages.push_back({ droppedImageId, dropPos });

			//		std::cout << "Dropped image with ID: " << droppedImageId << " at position (" << dropPos.x << ", " << dropPos.y << ")" << std::endl;
			//	}
			//	ImGui::EndDragDropTarget();
			//}
		}


		ImGui::End();
	};


	void Free() {};

	void Unload() {};
};