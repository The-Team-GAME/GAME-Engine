//-----------------------------------------------------------------------------------
// file:    UpgradeUIStateMachine.cpp
// project: Monkey Stole My GPA
//
// brief:   Implementation of the UpgradeUIStateMachine. Drives the upgrade flow
//          states (Idle/Available/Rolling/PickerOpen/Applying/Skipping) and
//          spawns/destroys entity-based UI elements for the card picker.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "UpgradeUIStateMachine.h"
#include "UpgradeManager.h"
#include "ECS/ECS.h"
#include "Components/Components.h"
#include "Resources.h"
#include "InputManager.h"
#include "Time.h"
#include "Logger.h"
#include "FileIO/Settings.h"
#include "FileIO/SettingsManager.h"

namespace Engine
{
    // ---- Singleton ----

    namespace { UpgradeUIStateMachine* g_upgrade_ui_sm = nullptr; }

    UpgradeUIStateMachine& UpgradeUIStateMachine::instance()
    {
        if (!g_upgrade_ui_sm) g_upgrade_ui_sm = new UpgradeUIStateMachine();
        return *g_upgrade_ui_sm;
    }

    void UpgradeUIStateMachine::destroy_instance()
    {
        delete g_upgrade_ui_sm;
        g_upgrade_ui_sm = nullptr;
    }

    // ---- State queries ----

    UpgradeFlowState UpgradeUIStateMachine::get_state() const
    {
        return state_;
    }

    void UpgradeUIStateMachine::set_upgrade_button(ECS::EntityID id)
    {
        upgrade_btn_id_ = id;
    }

    // ---- Transition logic ----

    void UpgradeUIStateMachine::transition_to(UpgradeFlowState new_state)
    {
        if (new_state == state_) return;
        on_exit(state_);
        UpgradeFlowState old = state_;
        state_ = new_state;
        on_enter(new_state);
        Log::info("[UpgradeUI] {} -> {}",
            static_cast<int>(old), static_cast<int>(new_state));
    }

    void UpgradeUIStateMachine::on_exit(UpgradeFlowState old_state)
    {
        switch (old_state)
        {
        case UpgradeFlowState::Available:
        {
            // Hide upgrade button when leaving Available
            if (upgrade_btn_id_ != 0 && ECS::is_entity_active(ECS::Entity(upgrade_btn_id_)))
            {
                auto* c = ECS::Entity(upgrade_btn_id_).get_component<Components::Color>();
                if (c) c->a = 0.0f;
            }
            break;
        }
        case UpgradeFlowState::PickerOpen:
        {
            // Destroy picker entities and resume time
            destroy_picker_entities();
            Time::set_time_scale(1.0f);
            break;
        }
        default:
            break;
        }
    }

    void UpgradeUIStateMachine::on_enter(UpgradeFlowState new_state)
    {
        auto& mgr = UpgradeManager::instance();

        switch (new_state)
        {
        case UpgradeFlowState::Available:
        {
            // Show upgrade button
            if (upgrade_btn_id_ != 0 && ECS::is_entity_active(ECS::Entity(upgrade_btn_id_)))
            {
                auto* c = ECS::Entity(upgrade_btn_id_).get_component<Components::Color>();
                if (c) c->a = 1.0f;
            }
            break;
        }
        case UpgradeFlowState::Rolling:
        {
            // Roll cards immediately
            mgr.roll_cards();
            break;
        }
        case UpgradeFlowState::PickerOpen:
        {
            // Pause time and spawn picker UI
            Time::set_time_scale(0.0001f);
            spawn_picker_entities();
            break;
        }
        case UpgradeFlowState::Applying:
        {
            // Apply the picked upgrade
            mgr.pick_card(picked_slot_);
            picked_slot_ = -1;
            break;
        }
        case UpgradeFlowState::Skipping:
        {
            // Skip cards
            mgr.skip_cards();
            break;
        }
        default:
            break;
        }
    }

    // ---- Per-frame update ----

    void UpgradeUIStateMachine::update()
    {
        auto& mgr = UpgradeManager::instance();

        switch (state_)
        {
        case UpgradeFlowState::Idle:
        {
            if (mgr.can_upgrade())
                transition_to(UpgradeFlowState::Available);
            break;
        }
        case UpgradeFlowState::Available:
        {
            if (!mgr.can_upgrade())
            {
                transition_to(UpgradeFlowState::Idle);
                break;
            }

            // Check for 'I' key or mouse click on upgrade button
            bool open_picker = Input::is_pressed(KeyCode::I);

            if (!open_picker && upgrade_btn_id_ != 0
                && ECS::is_entity_active(ECS::Entity(upgrade_btn_id_)))
            {
                auto* t = ECS::Entity(upgrade_btn_id_).get_component<Components::Transform2D>();
                if (t)
                {
                    glm::vec2 mouse = Input::get_mouse_position();
                    float hw = t->scale.x / 2.0f;
                    float hh = t->scale.y / 2.0f;
                    if (mouse.x >= t->position.x - hw && mouse.x <= t->position.x + hw &&
                        mouse.y >= t->position.y - hh && mouse.y <= t->position.y + hh)
                    {
                        if (Input::is_pressed(KeyCode::MouseLeft))
                            open_picker = true;
                    }
                }
            }

            if (open_picker)
                transition_to(UpgradeFlowState::Rolling);
            break;
        }
        case UpgradeFlowState::Rolling:
        {
            if (mgr.has_rolled_cards())
                transition_to(UpgradeFlowState::PickerOpen);
            else
                transition_to(UpgradeFlowState::Idle);
            break;
        }
        case UpgradeFlowState::PickerOpen:
        {
            glm::vec2 mouse = Input::get_mouse_position();

            // Hit-test card 0
            if (card0_id_ != 0 && ECS::is_entity_active(ECS::Entity(card0_id_)))
            {
                auto* t = ECS::Entity(card0_id_).get_component<Components::Transform2D>();
                if (t)
                {
                    float hw = t->scale.x / 2.0f;
                    float hh = t->scale.y / 2.0f;
                    if (mouse.x >= t->position.x - hw && mouse.x <= t->position.x + hw &&
                        mouse.y >= t->position.y - hh && mouse.y <= t->position.y + hh)
                    {
                        if (Input::is_pressed(KeyCode::MouseLeft))
                        {
                            picked_slot_ = 0;
                            transition_to(UpgradeFlowState::Applying);
                            break;
                        }
                    }
                }
            }

            // Hit-test card 1
            if (card1_id_ != 0 && ECS::is_entity_active(ECS::Entity(card1_id_)))
            {
                auto* t = ECS::Entity(card1_id_).get_component<Components::Transform2D>();
                if (t)
                {
                    float hw = t->scale.x / 2.0f;
                    float hh = t->scale.y / 2.0f;
                    if (mouse.x >= t->position.x - hw && mouse.x <= t->position.x + hw &&
                        mouse.y >= t->position.y - hh && mouse.y <= t->position.y + hh)
                    {
                        if (Input::is_pressed(KeyCode::MouseLeft))
                        {
                            picked_slot_ = 1;
                            transition_to(UpgradeFlowState::Applying);
                            break;
                        }
                    }
                }
            }

            // Hit-test skip button
            if (skip_id_ != 0 && ECS::is_entity_active(ECS::Entity(skip_id_)))
            {
                auto* t = ECS::Entity(skip_id_).get_component<Components::Transform2D>();
                if (t)
                {
                    float hw = t->scale.x / 2.0f;
                    float hh = t->scale.y / 2.0f;
                    if (mouse.x >= t->position.x - hw && mouse.x <= t->position.x + hw &&
                        mouse.y >= t->position.y - hh && mouse.y <= t->position.y + hh)
                    {
                        if (Input::is_pressed(KeyCode::MouseLeft))
                        {
                            transition_to(UpgradeFlowState::Skipping);
                            break;
                        }
                    }
                }
            }
            break;
        }
        case UpgradeFlowState::Applying:
        case UpgradeFlowState::Skipping:
        {
            // Re-evaluate after applying or skipping
            if (mgr.can_upgrade())
                transition_to(UpgradeFlowState::Available);
            else
                transition_to(UpgradeFlowState::Idle);
            break;
        }
        }
    }

    // ---- Picker entity management ----

    void UpgradeUIStateMachine::spawn_picker_entities()
    {
        auto& mgr = UpgradeManager::instance();
        auto& settings = FileIO::SettingsManager<FileIO::UpgradeSettings>::get();

        // Helper: map rarity name to a color
        auto rarity_color = [&](const std::string& rarity) -> Components::Color {
            constexpr float colors[][4] = {
                {0.75f, 0.75f, 0.75f, 1.0f},  // 0 Common  — grey
                {0.30f, 0.55f, 1.00f, 1.0f},  // 1 Rare    — blue
                {0.70f, 0.30f, 1.00f, 1.0f},  // 2 Epic    — purple
                {1.00f, 0.75f, 0.00f, 1.0f},  // 3 Legendary — gold
                {1.00f, 0.30f, 0.30f, 1.0f},  // 4 Mythic  — red
                {0.00f, 1.00f, 0.80f, 1.0f},  // 5+ teal
            };
            for (int i = 0; i < static_cast<int>(settings.rarities.size()); ++i)
            {
                if (settings.rarities[i].name == rarity)
                {
                    int ci = (i < 6) ? i : 5;
                    return Components::Color(colors[ci][0], colors[ci][1], colors[ci][2], colors[ci][3]);
                }
            }
            return Components::Color(0.5f, 0.5f, 0.5f, 1.0f);
        };

        auto rect = Resources::get_rect_texture();

        // Dimmer — full-screen dark overlay
        auto dimmer = ECS::create_entity()
            .add_component<Components::Transform2D>(800.0f, 450.0f, 0.0f, 1600.0f, 900.0f)
            .add_component<Components::Texture>(rect)
            .add_component<Components::Color>(0.0f, 0.0f, 0.0f, 0.6f)
            .add_component<Components::UI>()
            .add_to_layer("main_menu_4");
        dimmer_id_ = dimmer.get_id();

        // Card 0 — left
        int idx0 = mgr.get_rolled_card(0);
        Components::Color col0 = (idx0 >= 0 && idx0 < static_cast<int>(settings.upgrades.size()))
            ? rarity_color(settings.upgrades[idx0].rarity)
            : Components::Color(0.3f, 0.3f, 0.3f, 1.0f);

        auto c0 = ECS::create_entity()
            .add_component<Components::Transform2D>(560.0f, 450.0f, 0.0f, 270.0f, 220.0f)
            .add_component<Components::Texture>(rect)
            .add_component<Components::Color>(col0)
            .add_component<Components::UI>()
            .add_to_layer("main_menu_5");
        card0_id_ = c0.get_id();

        // Card 1 — right
        int idx1 = mgr.get_rolled_card(1);
        Components::Color col1 = (idx1 >= 0 && idx1 < static_cast<int>(settings.upgrades.size()))
            ? rarity_color(settings.upgrades[idx1].rarity)
            : Components::Color(0.3f, 0.3f, 0.3f, 1.0f);

        auto c1 = ECS::create_entity()
            .add_component<Components::Transform2D>(1040.0f, 450.0f, 0.0f, 270.0f, 220.0f)
            .add_component<Components::Texture>(rect)
            .add_component<Components::Color>(col1)
            .add_component<Components::UI>()
            .add_to_layer("main_menu_5");
        card1_id_ = c1.get_id();

        // Skip button — bottom center
        auto skip = ECS::create_entity()
            .add_component<Components::Transform2D>(800.0f, 680.0f, 0.0f, 160.0f, 50.0f)
            .add_component<Components::Texture>(rect)
            .add_component<Components::Color>(0.4f, 0.4f, 0.4f, 1.0f)
            .add_component<Components::UI>()
            .add_to_layer("main_menu_5");
        skip_id_ = skip.get_id();
    }

    void UpgradeUIStateMachine::destroy_picker_entities()
    {
        auto destroy_if_active = [](ECS::EntityID& id)
        {
            if (id != 0 && ECS::is_entity_active(ECS::Entity(id)))
                ECS::destroy_entity(ECS::Entity(id));
            id = 0;
        };

        destroy_if_active(dimmer_id_);
        destroy_if_active(card0_id_);
        destroy_if_active(card1_id_);
        destroy_if_active(skip_id_);
    }

    // ---- Reset ----

    void UpgradeUIStateMachine::reset()
    {
        if (state_ == UpgradeFlowState::PickerOpen)
        {
            destroy_picker_entities();
            Time::set_time_scale(1.0f);
        }
        state_ = UpgradeFlowState::Idle;
        picked_slot_ = -1;
    }

} // namespace Engine
