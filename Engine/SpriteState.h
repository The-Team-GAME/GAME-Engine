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
#include <iostream>
#include "FileExplorer.h"
#include "SpriteDropper.h"
#include "Timeline.h"
#include "Time.h"



namespace fs = std::filesystem;

// images selection pathway(currently unused)
//extern std::string selected_file_path;

// The directory where the project is located, selected by the user
extern std::string root_path;






class SpriteState : public Engine::GameState<SpriteState> {

private:

	Timeline timeline;  // adds timeline class to file

	int linearMonkeyImageId = -1;
	int nearestMonkeyImageId = -1;
	std::vector<int> loadedImageIds;
	std::vector<DroppedImage> droppedImages;

	bool showFileExplorerSource = false;
	bool fileCopied = false;

	struct ImageData {
		unsigned int texture_id;
		const char* name;
	};

	std::vector<ImageData> ImageAspectRatio;
	float spritePosX = 0.0f;


	//loads my images accordingly into the sprite manager
	void LoadAllAssets() {
		loadedImageIds.clear();
		fs::path assets_path = fs::current_path() / "Engine" / "Assets" / "images";
		if (!fs::exists(assets_path) || !fs::is_directory(assets_path)) {
			fs::create_directories(assets_path);
			return;
		}
		for (const auto& entry : fs::directory_iterator(assets_path)) {
			if (entry.is_regular_file()) {
				std::string extension = entry.path().extension().string();
				std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
				if (extension == ".jpg" || extension == ".png" || extension == ".jpeg") {
					int textureId = Engine::GRAPHICS.loadTexture(entry.path().string().c_str(), Graphics::MinifyingFilter::Nearest);
					if (textureId != -1) {
						loadedImageIds.push_back(textureId);
					}
				}
			}
		}
	}




public:


	void Load() {
		LoadAllAssets(); //loads updating assets within sprite dropper
	};



	void Init() {
		linearMonkeyImageId = Engine::GRAPHICS.loadTexture("Assets/images/monkey.png");
		nearestMonkeyImageId = Engine::GRAPHICS.loadTexture("Assets/images/monkey.png", Graphics::MinifyingFilter::Nearest);
		std::cout << "Current path is " << std::filesystem::current_path() << std::endl;
	};


	void Update() {

	

	};



	void Draw() {
		// Main scene texture drawing
		Engine::GRAPHICS.clearScreen(0.1f, 0.1f, 0.1f, 1.0f);
		Engine::GRAPHICS.drawTexture(linearMonkeyImageId, 200.0f, 150.0f, 150.0f, 210.0f);
		Engine::GRAPHICS.drawTexture(nearestMonkeyImageId, 400.0f, 150.0f, 150.0f, 210.0f);
		Engine::GRAPHICS.drawTextureRegion(
			nearestMonkeyImageId, 600.0f, 150.0f, 150.0f, 210.0f, spritePosX += Engine::Time::get_fixed_dt() * 10.0f, 0.0f, 15.0f, 21.0f
		);



		// This loop draws the dropped images at their final position
		for (const auto& droppedImage : droppedImages) {
			//Engine::GRAPHICS.drawTexture(droppedImage.textureId, droppedImage.position.x, droppedImage.position.y, 150.0f, 210.0f);
			Engine::GRAPHICS.drawTexture(droppedImage.textureId, 600.f, 500.f, 150.0f, 210.0f);
		}

		timeline.Draw();

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

		//calls the sprite dropper function :>
		SpriteDropper(loadedImageIds, droppedImages);

	};

	void Free() {};
	void Unload() {};
};