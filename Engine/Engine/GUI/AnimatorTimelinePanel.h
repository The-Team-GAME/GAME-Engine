//-----------------------------------------------------------------------------------
// file:    AnimatorTimelinePanel.h
// project: Monkey Stole My GPA
//
// brief:   ImGui panel for editing property animation clips within state machine
//          states. Provides a visual timeline with keyframe editing, track management,
//          easing selection, and playback preview. Allows animating any numeric
//          component field (float, int, Vec2D, Color) over time.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include <string>

namespace Engine::ECS {
  class Entity;
}

namespace AnimatorTimeline {

  /// Open the Animator Timeline panel for a specific state's property animation clip
  void Open(Engine::ECS::Entity entity, const std::string& state_name);

}

/// Main ImGui draw function for the Animator Timeline panel
void AnimatorTimelinePanel();
