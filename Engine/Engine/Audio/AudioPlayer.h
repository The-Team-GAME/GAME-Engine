//-----------------------------------------------------------------------------------
// file:    AudioPlayer.h
// project: Monkey Stole My GPA
//
// author:  Chew Shee Yang (100%)
//          Primary author
//
// email:   chew.s@digipen.edu
//
// brief:   Public interface for the FMOD-based audio system. Provides sound
//          loading, playback control, and spatial audio functionality.
//          Supports 2D and 3D audio positioning with volume and looping controls.
//          Integrates with the resource management system for automatic cleanup.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#pragma once

#include "../Math/Vector2D.h"
#include "../TypeDef.h"
#include "../EngineExport.h"
#include <memory>
#include <string>

namespace Engine {
  namespace Audio {

    // Forward declarations
    struct SoundData;
    using Sound = std::shared_ptr<SoundData>;

    // Audio channel handle (similar to how you handle textures)
    using ChannelHandle = int;
    inline constexpr ChannelHandle INVALID_CHANNEL = -1;

    // Initialization and cleanup
    ENGINE_API bool init();
    ENGINE_API bool exit();
    ENGINE_API void update();

    // Playback functions
    ENGINE_API ChannelHandle play_sound(Sound sound, f32 volume = 1.0f, bool loop = false);
    ENGINE_API ChannelHandle play_sound_at(Sound sound, const Math::Vec2D& position, f32 volume = 1.0f, bool loop = false);

    ENGINE_API void stop_channel(ChannelHandle channel);
    ENGINE_API void pause_channel(ChannelHandle channel);
    ENGINE_API void resume_channel(ChannelHandle channel);

    // Channel properties
    ENGINE_API void set_channel_volume(ChannelHandle channel, f32 volume);
    ENGINE_API void set_channel_position(ChannelHandle channel, const Math::Vec2D& position);
    ENGINE_API bool is_channel_playing(ChannelHandle channel);

    // Global audio settings
    ENGINE_API void set_master_volume(f32 volume);
    ENGINE_API void set_bgm_volume(f32 volume);
    ENGINE_API void set_sfx_volume(f32 volume);
    ENGINE_API void set_listener_position(const Math::Vec2D& position);
    ENGINE_API void set_listener_forward(const Math::Vec2D& forward);

    ENGINE_API f32 get_master_volume();
    ENGINE_API f32 get_bgm_volume();
    ENGINE_API f32 get_sfx_volume();
    ENGINE_API Math::Vec2D get_listener_position();

    // Stop all sounds
    ENGINE_API void stop_all_sounds();
    ENGINE_API void pause_all_sounds();
    ENGINE_API void resume_all_sounds();

  } // namespace Audio
} // namespace Engine