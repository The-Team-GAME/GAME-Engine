#pragma once

#include "SceneManager.h"
#include "GameProjectManager.h"
#include "GameProjectScaffolder.h"
#include "Logger.h"

#include <imgui.h>
#include <vector>
#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>

namespace Engine
{
  class GameSelectorState : public Scene
  {
  public:
    void load() override {}
    void init() override
    {
      m_games = GameProjectManager::discover_games();
      Log::info("[GameSelector] Found {} game(s)", m_games.size());
      m_new_game_name[0] = '\0';
    }

    void update() override
    {
      ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
        ImGui::GetIO().DisplaySize.y * 0.5f),
        ImGuiCond_Always, ImVec2(0.5f, 0.5f));
      ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_Always);

      ImGui::Begin("Game Selector", nullptr,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize);

      ImGui::Text("Select a game to launch:");
      ImGui::Separator();

      if (m_games.empty())
      {
        ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "No games found in Games/ folder");
      }
      else
      {
        for (auto& game : m_games)
        {
          bool is_current = (game.name == GameProjectManager::get_current_game_name());
          if (is_current)
          {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1));
          }

          float avail = ImGui::GetContentRegionAvail().x;
          float sln_btn_w = 80.0f;
          float game_btn_w = avail - sln_btn_w - ImGui::GetStyle().ItemSpacing.x;

          if (ImGui::Button(game.name.c_str(), ImVec2(game_btn_w, 30)))
          {
            m_selected = game.name;
          }

          if (is_current)
            ImGui::PopStyleColor(2);

          ImGui::SameLine();
          bool has_sln = !game.sln_path.empty() && std::filesystem::exists(game.sln_path);
          if (!has_sln) ImGui::BeginDisabled();
          if (ImGui::Button(("Open .sln##" + game.name).c_str(), ImVec2(sln_btn_w, 30)))
          {
            ShellExecuteW(nullptr, L"open", game.sln_path.wstring().c_str(),
              nullptr, nullptr, SW_SHOWNORMAL);
          }
          if (!has_sln) ImGui::EndDisabled();
        }
      }

      ImGui::Separator();
      ImGui::Text("Create New Game Project:");
      ImGui::InputText("##newgame", m_new_game_name, sizeof(m_new_game_name));
      ImGui::SameLine();
      if (ImGui::Button("Create") && m_new_game_name[0] != '\0')
      {
        std::string name(m_new_game_name);
        if (GameProjectScaffolder::create_project(name))
        {
          m_games = GameProjectManager::discover_games();
          m_new_game_name[0] = '\0';
        }
      }

      ImGui::End();

      // Launch selected game outside of ImGui rendering
      if (!m_selected.empty())
      {
        std::string to_launch = m_selected;
        m_selected.clear();

        if (GameProjectManager::switch_game(to_launch))
        {
          auto scene_names = SceneManager::get_all_scene_names();
          if (!scene_names.empty())
          {
            SceneManager::load_scene(scene_names[0]);
          }
        }
      }
    }

    void draw() override {}
    void free() override {}
    void unload() override {}

  private:
    std::vector<GameProject> m_games;
    std::string m_selected;
    char m_new_game_name[128]{};
  };
}
