//-----------------------------------------------------------------------------------
// file:    AudioComponents.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   ECS component definitions for audio.
//          Includes Sound component for handling entity audio.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Audio/AudioPlayer.h"
#include "../Audio/AudioPlayerInternal.h"
#include "../Reflection/ReflectionLibrary.h"
#include "../ECS/Macros.h"
#include "../Resources.h"
#include <string>

namespace Components
{
  struct Sound
  {
    Engine::Audio::Sound sound_handle;

    Sound() : sound_handle(nullptr) {}
    Sound(Engine::Audio::Sound snd) : sound_handle(snd) {}

    // Helper to play the sound directly from the component
    Engine::Audio::ChannelHandle play(f32 volume = 1.0f, bool loop = false) const {
      if (sound_handle) {
        return Engine::Audio::play_sound(sound_handle, volume, loop);
      }
      return Engine::Audio::INVALID_CHANNEL;
    }

    struct Reflector {
      std::string path;

      static Reflector reflect(const Sound& snd) {
        Reflector r;
        if (snd.sound_handle) {
          r.path = Engine::Resources::get_relative_path(snd.sound_handle->path);
        }
        return r;
      }

      static Sound create(const Reflector& r) {
        return Engine::Resources::load_sound(r.path);
      }
    };
  };
}

REGISTER_COMPONENT_CUSTOM(Components::Sound, path)
