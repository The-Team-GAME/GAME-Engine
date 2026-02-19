//-----------------------------------------------------------------------------------
// file:    PostProcessing.cpp
// project: Monkey Stole My GPA
//
// brief:   Implementation of the modular post-processing pipeline. Manages FBOs
//          for scene capture and Kawase bloom (downsample/upsample mip chain),
//          3D LUT texture for color grading, and gamma correction. All effects
//          are composited in a single final pass.
//
// Copyright  2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "PostProcessing.h"
#include "Shader.h"
#include "CubeLUT.h"
#include "../Logger.h"
#include "../Time.h"
#include "../FileIO/Helpers.h"

#include <algorithm>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

namespace Engine::Graphics::PostProcessing {

  // ===== Internal state =====
  namespace {
    Settings current_settings;

    // Scene FBO
    GLuint scene_fbo = 0;
    GLuint scene_color_texture = 0;

    // MSAA FBO (multisampled scene target)
    GLuint msaa_fbo = 0;
    GLuint msaa_color_renderbuffer = 0;
    GLuint msaa_depth_renderbuffer = 0;
    int active_msaa_samples = 0;

    // Bloom mip chain
    static constexpr int MAX_BLOOM_MIPS = 8;
    struct BloomMip {
      GLuint fbo = 0;
      GLuint texture = 0;
      int width = 0;
      int height = 0;
    };
    BloomMip bloom_mips[MAX_BLOOM_MIPS];
    int bloom_mip_count = 0;

    // LUT
    GLuint lut_texture = 0;
    float lut_size = 0.0f;

    // Fullscreen quad
    GLuint quad_vao = 0;
    GLuint quad_vbo = 0;

    // Shaders
    std::shared_ptr<Shader> downsample_shader;
    std::shared_ptr<Shader> upsample_shader;
    std::shared_ptr<Shader> composite_shader;
    std::shared_ptr<Shader> sharpen_shader;

    // Sharpen pass FBO (ping-pong)
    GLuint sharpen_fbo = 0;
    GLuint sharpen_texture = 0;

    // Bloom dirt texture
    GLuint bloom_dirt_texture = 0;

    // Dimensions
    int fb_width = 1600;
    int fb_height = 900;

    // Previous FBO to restore after scene capture
    GLint previous_fbo = 0;
    GLint previous_viewport[4] = { 0 };

    //---------------------------------------------------------------
    // Create the fullscreen quad geometry
    //---------------------------------------------------------------
    void create_quad() {
      float vertices[] = {
        // position    // texcoord
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,

        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
      };

      glGenVertexArrays(1, &quad_vao);
      glGenBuffers(1, &quad_vbo);

      glBindVertexArray(quad_vao);
      glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

      // position
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
      // texcoord
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

      glBindVertexArray(0);
    }

    //---------------------------------------------------------------
    // Render the fullscreen quad
    //---------------------------------------------------------------
    void render_quad() {
      glBindVertexArray(quad_vao);
      glDrawArrays(GL_TRIANGLES, 0, 6);
      glBindVertexArray(0);
    }

    //---------------------------------------------------------------
    // Create a color FBO (no depth needed for post-processing)
    //---------------------------------------------------------------
    void create_fbo(GLuint& fbo, GLuint& texture, int w, int h) {
      glGenFramebuffers(1, &fbo);
      glBindFramebuffer(GL_FRAMEBUFFER, fbo);

      glGenTextures(1, &texture);
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, nullptr);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        Log::error("PostProcessing: Framebuffer not complete ({}x{})", w, h);
      }

      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    //---------------------------------------------------------------
    // Create the bloom mip chain
    //---------------------------------------------------------------
    void create_bloom_mips() {
      int mip_w = fb_width / 2;
      int mip_h = fb_height / 2;
      bloom_mip_count = 0;

      for (int i = 0; i < MAX_BLOOM_MIPS && mip_w >= 2 && mip_h >= 2; ++i) {
        create_fbo(bloom_mips[i].fbo, bloom_mips[i].texture, mip_w, mip_h);
        bloom_mips[i].width = mip_w;
        bloom_mips[i].height = mip_h;
        bloom_mip_count++;
        mip_w /= 2;
        mip_h /= 2;
      }
    }

    //---------------------------------------------------------------
    // Destroy bloom mips
    //---------------------------------------------------------------
    void destroy_bloom_mips() {
      for (int i = 0; i < bloom_mip_count; ++i) {
        if (bloom_mips[i].fbo) {
          glDeleteFramebuffers(1, &bloom_mips[i].fbo);
          bloom_mips[i].fbo = 0;
        }
        if (bloom_mips[i].texture) {
          glDeleteTextures(1, &bloom_mips[i].texture);
          bloom_mips[i].texture = 0;
        }
      }
      bloom_mip_count = 0;
    }

    //---------------------------------------------------------------
    // Upload LUT data to a 3D texture
    //---------------------------------------------------------------
    void upload_lut(const CubeLUTData& lut_data) {
      if (lut_texture) {
        glDeleteTextures(1, &lut_texture);
        lut_texture = 0;
      }

      glGenTextures(1, &lut_texture);
      glBindTexture(GL_TEXTURE_3D, lut_texture);

      glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F,
        lut_data.size, lut_data.size, lut_data.size,
        0, GL_RGB, GL_FLOAT, lut_data.data.data());

      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

      lut_size = static_cast<float>(lut_data.size);

      glBindTexture(GL_TEXTURE_3D, 0);
    }

    //---------------------------------------------------------------
    // Create MSAA FBO with multisampled renderbuffers
    //---------------------------------------------------------------
    void create_msaa_fbo(int samples) {
      // Clamp samples to GPU max
      GLint max_samples = 0;
      glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
      samples = std::min(samples, static_cast<int>(max_samples));
      if (samples < 2) samples = 2;

      glGenFramebuffers(1, &msaa_fbo);
      glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo);

      // Color renderbuffer (multisampled, RGB16F to match scene FBO)
      glGenRenderbuffers(1, &msaa_color_renderbuffer);
      glBindRenderbuffer(GL_RENDERBUFFER, msaa_color_renderbuffer);
      glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGB16F, fb_width, fb_height);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaa_color_renderbuffer);

      // Depth renderbuffer (multisampled — required for correct depth testing)
      glGenRenderbuffers(1, &msaa_depth_renderbuffer);
      glBindRenderbuffer(GL_RENDERBUFFER, msaa_depth_renderbuffer);
      glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, fb_width, fb_height);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, msaa_depth_renderbuffer);

      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        Log::error("PostProcessing: MSAA framebuffer not complete ({}x{}, {}x MSAA)", fb_width, fb_height, samples);
      }

      active_msaa_samples = samples;
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    //---------------------------------------------------------------
    // Destroy MSAA FBO
    //---------------------------------------------------------------
    void destroy_msaa_fbo() {
      if (msaa_fbo) { glDeleteFramebuffers(1, &msaa_fbo); msaa_fbo = 0; }
      if (msaa_color_renderbuffer) { glDeleteRenderbuffers(1, &msaa_color_renderbuffer); msaa_color_renderbuffer = 0; }
      if (msaa_depth_renderbuffer) { glDeleteRenderbuffers(1, &msaa_depth_renderbuffer); msaa_depth_renderbuffer = 0; }
      active_msaa_samples = 0;
    }

  } // anonymous namespace

  // ===== Public API =====

  bool init() {
    // Create fullscreen quad
    create_quad();

    // Load shaders
    downsample_shader = Shader::load_from_file("Assets/shaders/postprocess.vert", "Assets/shaders/bloom_downsample.frag");
    if (!downsample_shader || !downsample_shader->is_valid()) {
      Log::error("PostProcessing: Failed to load bloom downsample shader");
      return false;
    }

    upsample_shader = Shader::load_from_file("Assets/shaders/postprocess.vert", "Assets/shaders/bloom_upsample.frag");
    if (!upsample_shader || !upsample_shader->is_valid()) {
      Log::error("PostProcessing: Failed to load bloom upsample shader");
      return false;
    }

    composite_shader = Shader::load_from_file("Assets/shaders/postprocess.vert", "Assets/shaders/postprocess.frag");
    if (!composite_shader || !composite_shader->is_valid()) {
      Log::error("PostProcessing: Failed to load composite shader");
      return false;
    }

    sharpen_shader = Shader::load_from_file("Assets/shaders/postprocess.vert", "Assets/shaders/sharpen.frag");
    if (!sharpen_shader || !sharpen_shader->is_valid()) {
      Log::error("PostProcessing: Failed to load sharpen shader");
      return false;
    }

    // Create scene FBO
    create_fbo(scene_fbo, scene_color_texture, fb_width, fb_height);

    // Create sharpen FBO (same size as scene)
    create_fbo(sharpen_fbo, sharpen_texture, fb_width, fb_height);

    // Create bloom mip chain
    create_bloom_mips();

    // Create MSAA FBO (default 4x)
    create_msaa_fbo(current_settings.msaa_samples);

    // Create identity LUT as default
    CubeLUTData identity = generate_identity_lut(16);
    upload_lut(identity);

    Log::info("PostProcessing: Initialized ({}x{}, {} bloom mips, {}x MSAA)", fb_width, fb_height, bloom_mip_count, active_msaa_samples);
    return true;
  }

  void exit() {
    // Scene FBO
    if (scene_fbo) { glDeleteFramebuffers(1, &scene_fbo); scene_fbo = 0; }
    if (scene_color_texture) { glDeleteTextures(1, &scene_color_texture); scene_color_texture = 0; }

    // MSAA
    destroy_msaa_fbo();

    // Bloom
    destroy_bloom_mips();

    // LUT
    if (lut_texture) { glDeleteTextures(1, &lut_texture); lut_texture = 0; }

    // Quad
    if (quad_vao) { glDeleteVertexArrays(1, &quad_vao); quad_vao = 0; }
    if (quad_vbo) { glDeleteBuffers(1, &quad_vbo); quad_vbo = 0; }

    // Sharpen FBO
    if (sharpen_fbo) { glDeleteFramebuffers(1, &sharpen_fbo); sharpen_fbo = 0; }
    if (sharpen_texture) { glDeleteTextures(1, &sharpen_texture); sharpen_texture = 0; }

    // Bloom dirt texture
    if (bloom_dirt_texture) { glDeleteTextures(1, &bloom_dirt_texture); bloom_dirt_texture = 0; }

    // Shaders
    downsample_shader.reset();
    upsample_shader.reset();
    composite_shader.reset();
    sharpen_shader.reset();

    Log::info("PostProcessing: Cleaned up");
  }

  void begin_scene_capture() {
    if (!current_settings.enabled) return;

    // Save current FBO and viewport so we can restore them later
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous_fbo);
    glGetIntegerv(GL_VIEWPORT, previous_viewport);

    // Recreate MSAA FBO if sample count changed
    if (current_settings.msaa_enabled && active_msaa_samples != current_settings.msaa_samples) {
      destroy_msaa_fbo();
      create_msaa_fbo(current_settings.msaa_samples);
    }

    // Bind either the MSAA or regular scene FBO
    if (current_settings.msaa_enabled && msaa_fbo) {
      glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo);
      glEnable(GL_MULTISAMPLE);
    }
    else {
      glBindFramebuffer(GL_FRAMEBUFFER, scene_fbo);
    }

    glViewport(0, 0, fb_width, fb_height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void end_scene_capture_and_apply(GLuint target_fbo) {
    if (!current_settings.enabled) return;

    // If target_fbo is 0, restore to whatever FBO was bound before scene capture
    // (supports ViewportPanel FBO in editor mode)
    GLuint final_target = (target_fbo == 0) ? static_cast<GLuint>(previous_fbo) : target_fbo;

    // ===== MSAA RESOLVE =====
    // Blit multisampled FBO → regular scene FBO (resolves MSAA)
    if (current_settings.msaa_enabled && msaa_fbo) {
      glBindFramebuffer(GL_READ_FRAMEBUFFER, msaa_fbo);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, scene_fbo);
      glBlitFramebuffer(
        0, 0, fb_width, fb_height,
        0, 0, fb_width, fb_height,
        GL_COLOR_BUFFER_BIT, GL_LINEAR
      );
      glDisable(GL_MULTISAMPLE);
    }

    // Disable blending for post-processing passes
    glDisable(GL_BLEND);

    // ===== BLOOM =====
    if (current_settings.bloom_enabled && bloom_mip_count > 0) {
      int iterations = std::min(current_settings.bloom_iterations, bloom_mip_count);

      // --- Downsample passes ---
      downsample_shader->use();
      downsample_shader->set_int("u_source", 0);

      // First pass: from scene texture (with brightness threshold)
      GLuint source_texture = scene_color_texture;
      int source_w = fb_width;
      int source_h = fb_height;

      for (int i = 0; i < iterations; ++i) {
        downsample_shader->set_vec2("u_texel_size", glm::vec2(1.0f / source_w, 1.0f / source_h));
        downsample_shader->set_bool("u_first_pass", i == 0);
        if (i == 0) {
          downsample_shader->set_float("u_threshold", current_settings.bloom_threshold);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, bloom_mips[i].fbo);
        glViewport(0, 0, bloom_mips[i].width, bloom_mips[i].height);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, source_texture);

        render_quad();

        source_texture = bloom_mips[i].texture;
        source_w = bloom_mips[i].width;
        source_h = bloom_mips[i].height;
      }

      // --- Upsample passes ---
      upsample_shader->use();
      upsample_shader->set_int("u_source", 0);

      // Enable additive blending for upsampling
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE);

      for (int i = iterations - 1; i > 0; --i) {
        int target_idx = i - 1;
        upsample_shader->set_vec2("u_texel_size",
          glm::vec2(1.0f / bloom_mips[i].width, 1.0f / bloom_mips[i].height));

        glBindFramebuffer(GL_FRAMEBUFFER, bloom_mips[target_idx].fbo);
        glViewport(0, 0, bloom_mips[target_idx].width, bloom_mips[target_idx].height);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, bloom_mips[i].texture);

        render_quad();
      }

      // Restore normal blending
      glDisable(GL_BLEND);
    }

    // ===== FINAL COMPOSITE PASS =====
    // If sharpen is enabled, render composite to sharpen FBO first, then sharpen to final target
    GLuint composite_target = final_target;
    if (current_settings.sharpen_enabled && sharpen_fbo) {
      composite_target = sharpen_fbo;
      glBindFramebuffer(GL_FRAMEBUFFER, sharpen_fbo);
      glViewport(0, 0, fb_width, fb_height);
    }
    else {
      glBindFramebuffer(GL_FRAMEBUFFER, final_target);
      glViewport(previous_viewport[0], previous_viewport[1], previous_viewport[2], previous_viewport[3]);
    }

    composite_shader->use();

    // Scene texture (unit 0)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene_color_texture);
    composite_shader->set_int("u_scene", 0);

    // Bloom texture (unit 1) - use first mip (full accumulated bloom)
    glActiveTexture(GL_TEXTURE1);
    if (current_settings.bloom_enabled && bloom_mip_count > 0) {
      glBindTexture(GL_TEXTURE_2D, bloom_mips[0].texture);
    }
    else {
      glBindTexture(GL_TEXTURE_2D, 0); // No bloom
    }
    composite_shader->set_int("u_bloom", 1);

    // LUT texture (unit 2)
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, lut_texture);
    composite_shader->set_int("u_lut", 2);

    // Bloom dirt texture (unit 3)
    glActiveTexture(GL_TEXTURE3);
    if (current_settings.bloom_dirt_enabled && bloom_dirt_texture) {
      glBindTexture(GL_TEXTURE_2D, bloom_dirt_texture);
    }
    else {
      glBindTexture(GL_TEXTURE_2D, 0);
    }
    composite_shader->set_int("u_bloom_dirt", 3);

    // Uniforms
    composite_shader->set_float("u_gamma", current_settings.gamma);
    composite_shader->set_bool("u_bloom_enabled", current_settings.bloom_enabled);
    composite_shader->set_float("u_bloom_intensity", current_settings.bloom_intensity);
    composite_shader->set_bool("u_lut_enabled", current_settings.lut_enabled);
    composite_shader->set_float("u_lut_intensity", current_settings.lut_intensity);
    composite_shader->set_float("u_lut_size", lut_size);

    // Bloom enhancements
    composite_shader->set_vec3("u_bloom_tint", current_settings.bloom_tint);
    composite_shader->set_bool("u_bloom_dirt_enabled", current_settings.bloom_dirt_enabled && bloom_dirt_texture != 0);
    composite_shader->set_float("u_bloom_anamorphic", current_settings.bloom_anamorphic);

    // Chromatic Aberration
    composite_shader->set_bool("u_chromatic_aberration_enabled", current_settings.chromatic_aberration_enabled);
    composite_shader->set_float("u_chromatic_aberration_intensity", current_settings.chromatic_aberration_intensity);
    composite_shader->set_int("u_chromatic_aberration_samples", current_settings.chromatic_aberration_samples);
    composite_shader->set_bool("u_chromatic_aberration_edge_only", current_settings.chromatic_aberration_edge_only);

    // Vignette
    composite_shader->set_bool("u_vignette_enabled", current_settings.vignette_enabled);
    composite_shader->set_float("u_vignette_intensity", current_settings.vignette_intensity);
    composite_shader->set_float("u_vignette_radius", current_settings.vignette_radius);
    composite_shader->set_vec3("u_vignette_color", current_settings.vignette_color);
    composite_shader->set_float("u_vignette_roundness", current_settings.vignette_roundness);
    composite_shader->set_float("u_vignette_smoothness", current_settings.vignette_smoothness);

    // Color Quantization
    composite_shader->set_bool("u_quantize_enabled", current_settings.quantize_enabled);
    composite_shader->set_float("u_quantize_levels", static_cast<float>(current_settings.quantize_levels));
    composite_shader->set_bool("u_quantize_dither_enabled", current_settings.quantize_dither_enabled);
    composite_shader->set_vec3("u_quantize_levels_rgb", glm::vec3(current_settings.quantize_levels_rgb));
    composite_shader->set_int("u_quantize_palette", current_settings.quantize_palette);

    // CRT Emulation
    composite_shader->set_bool("u_crt_enabled", current_settings.crt_enabled);
    composite_shader->set_float("u_crt_scanline_intensity", current_settings.crt_scanline_intensity);
    composite_shader->set_float("u_crt_curvature", current_settings.crt_curvature);
    composite_shader->set_float("u_crt_scanline_count", current_settings.crt_scanline_count);
    composite_shader->set_vec2("u_resolution", glm::vec2(static_cast<float>(fb_width), static_cast<float>(fb_height)));
    composite_shader->set_bool("u_crt_subpixel_enabled", current_settings.crt_subpixel_enabled);
    composite_shader->set_bool("u_crt_glow_enabled", current_settings.crt_glow_enabled);
    composite_shader->set_float("u_crt_glow_intensity", current_settings.crt_glow_intensity);
    composite_shader->set_float("u_crt_corner_radius", current_settings.crt_corner_radius);
    composite_shader->set_float("u_crt_flicker_intensity", current_settings.crt_flicker_intensity);

    // Time (for animated effects)
    composite_shader->set_float("u_time", Engine::Time::get_time());

    // Film Grain
    composite_shader->set_bool("u_film_grain_enabled", current_settings.film_grain_enabled);
    composite_shader->set_float("u_film_grain_intensity", current_settings.film_grain_intensity);
    composite_shader->set_float("u_film_grain_size", current_settings.film_grain_size);

    // Tone Mapping
    composite_shader->set_bool("u_tonemapping_enabled", current_settings.tonemapping_enabled);
    composite_shader->set_int("u_tonemapping_mode", current_settings.tonemapping_mode);
    composite_shader->set_float("u_tonemapping_exposure", current_settings.tonemapping_exposure);

    // Effect variant
    composite_shader->set_int("u_effect_variant", current_settings.effect_variant);

    render_quad();

    // ===== SHARPEN PASS =====
    if (current_settings.sharpen_enabled && sharpen_fbo) {
      glBindFramebuffer(GL_FRAMEBUFFER, final_target);
      glViewport(previous_viewport[0], previous_viewport[1], previous_viewport[2], previous_viewport[3]);

      sharpen_shader->use();

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, sharpen_texture);
      sharpen_shader->set_int("u_scene", 0);
      sharpen_shader->set_float("u_sharpen_intensity", current_settings.sharpen_intensity);

      render_quad();
    }

    // Reset to texture unit 0
    glActiveTexture(GL_TEXTURE0);

    // Restore blending state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  Settings& get_settings() {
    return current_settings;
  }

  void set_settings(const Settings& settings) {
    current_settings = settings;
  }

  bool load_lut_from_cube(const std::string& filepath) {
    CubeLUTData data = parse_cube_lut(filepath);
    if (data.size == 0) {
      return false;
    }
    upload_lut(data);
    Log::info("PostProcessing: Loaded LUT from {} ({}x{}x{})", filepath, data.size, data.size, data.size);
    return true;
  }

  void load_bloom_dirt_texture(const std::string& filepath) {
    if (bloom_dirt_texture) {
      glDeleteTextures(1, &bloom_dirt_texture);
      bloom_dirt_texture = 0;
    }

    int w, h, channels;
    unsigned char* data = stbi_load(filepath.c_str(), &w, &h, &channels, 3);
    if (!data) {
      Log::error("PostProcessing: Failed to load bloom dirt texture: {}", filepath);
      return;
    }

    glGenTextures(1, &bloom_dirt_texture);
    glBindTexture(GL_TEXTURE_2D, bloom_dirt_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    Log::info("PostProcessing: Loaded bloom dirt texture from {} ({}x{})", filepath, w, h);
  }

  bool is_layer_affected(const std::string& layer_name) {
    // Empty list = all layers affected
    if (current_settings.affected_layers.empty()) return true;

    for (const auto& name : current_settings.affected_layers) {
      if (name == layer_name) return true;
    }
    return false;
  }

  void resize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (width == fb_width && height == fb_height) return;

    fb_width = width;
    fb_height = height;

    // Resize scene FBO
    glBindTexture(GL_TEXTURE_2D, scene_color_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, fb_width, fb_height, 0, GL_RGB, GL_FLOAT, nullptr);

    // Resize sharpen FBO
    if (sharpen_texture) {
      glBindTexture(GL_TEXTURE_2D, sharpen_texture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, fb_width, fb_height, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    // Recreate bloom mips
    destroy_bloom_mips();
    create_bloom_mips();

    // Recreate MSAA FBO at new size
    if (active_msaa_samples > 0) {
      destroy_msaa_fbo();
      create_msaa_fbo(current_settings.msaa_samples);
    }

    Log::info("PostProcessing: Resized to {}x{} ({} bloom mips, {}x MSAA)", fb_width, fb_height, bloom_mip_count, active_msaa_samples);
  }

  int get_width() { return fb_width; }
  int get_height() { return fb_height; }

  // ===== Serialization =====

  std::string serialize_settings_to_json(const Settings& s) {
    rapidjson::Document doc;
    doc.SetObject();
    auto& a = doc.GetAllocator();

    doc.AddMember("enabled", s.enabled, a);
    doc.AddMember("gamma", s.gamma, a);

    // Bloom
    doc.AddMember("bloom_enabled", s.bloom_enabled, a);
    doc.AddMember("bloom_threshold", s.bloom_threshold, a);
    doc.AddMember("bloom_intensity", s.bloom_intensity, a);
    doc.AddMember("bloom_iterations", s.bloom_iterations, a);
    {
      rapidjson::Value arr(rapidjson::kArrayType);
      arr.PushBack(s.bloom_tint.x, a).PushBack(s.bloom_tint.y, a).PushBack(s.bloom_tint.z, a);
      doc.AddMember("bloom_tint", arr, a);
    }
    doc.AddMember("bloom_dirt_enabled", s.bloom_dirt_enabled, a);
    doc.AddMember("bloom_anamorphic", s.bloom_anamorphic, a);

    // LUT
    doc.AddMember("lut_enabled", s.lut_enabled, a);
    doc.AddMember("lut_intensity", s.lut_intensity, a);

    // Vignette
    doc.AddMember("vignette_enabled", s.vignette_enabled, a);
    doc.AddMember("vignette_intensity", s.vignette_intensity, a);
    doc.AddMember("vignette_radius", s.vignette_radius, a);
    {
      rapidjson::Value arr(rapidjson::kArrayType);
      arr.PushBack(s.vignette_color.x, a).PushBack(s.vignette_color.y, a).PushBack(s.vignette_color.z, a);
      doc.AddMember("vignette_color", arr, a);
    }
    doc.AddMember("vignette_roundness", s.vignette_roundness, a);
    doc.AddMember("vignette_smoothness", s.vignette_smoothness, a);

    // Chromatic Aberration
    doc.AddMember("chromatic_aberration_enabled", s.chromatic_aberration_enabled, a);
    doc.AddMember("chromatic_aberration_intensity", s.chromatic_aberration_intensity, a);
    doc.AddMember("chromatic_aberration_samples", s.chromatic_aberration_samples, a);
    doc.AddMember("chromatic_aberration_edge_only", s.chromatic_aberration_edge_only, a);

    // CRT
    doc.AddMember("crt_enabled", s.crt_enabled, a);
    doc.AddMember("crt_scanline_intensity", s.crt_scanline_intensity, a);
    doc.AddMember("crt_curvature", s.crt_curvature, a);
    doc.AddMember("crt_scanline_count", s.crt_scanline_count, a);
    doc.AddMember("crt_subpixel_enabled", s.crt_subpixel_enabled, a);
    doc.AddMember("crt_glow_enabled", s.crt_glow_enabled, a);
    doc.AddMember("crt_glow_intensity", s.crt_glow_intensity, a);
    doc.AddMember("crt_corner_radius", s.crt_corner_radius, a);
    doc.AddMember("crt_flicker_intensity", s.crt_flicker_intensity, a);

    // Quantization
    doc.AddMember("quantize_enabled", s.quantize_enabled, a);
    doc.AddMember("quantize_levels", s.quantize_levels, a);
    doc.AddMember("quantize_dither_enabled", s.quantize_dither_enabled, a);
    {
      rapidjson::Value arr(rapidjson::kArrayType);
      arr.PushBack(s.quantize_levels_rgb.x, a).PushBack(s.quantize_levels_rgb.y, a).PushBack(s.quantize_levels_rgb.z, a);
      doc.AddMember("quantize_levels_rgb", arr, a);
    }
    doc.AddMember("quantize_palette", s.quantize_palette, a);

    // Film Grain
    doc.AddMember("film_grain_enabled", s.film_grain_enabled, a);
    doc.AddMember("film_grain_intensity", s.film_grain_intensity, a);
    doc.AddMember("film_grain_size", s.film_grain_size, a);

    // Tone Mapping
    doc.AddMember("tonemapping_enabled", s.tonemapping_enabled, a);
    doc.AddMember("tonemapping_mode", s.tonemapping_mode, a);
    doc.AddMember("tonemapping_exposure", s.tonemapping_exposure, a);

    // Sharpen
    doc.AddMember("sharpen_enabled", s.sharpen_enabled, a);
    doc.AddMember("sharpen_intensity", s.sharpen_intensity, a);

    // Effect variant
    doc.AddMember("effect_variant", s.effect_variant, a);

    // MSAA
    doc.AddMember("msaa_enabled", s.msaa_enabled, a);
    doc.AddMember("msaa_samples", s.msaa_samples, a);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
  }

  Settings deserialize_settings_from_json(const std::string& json_str) {
    Settings s;
    rapidjson::Document doc;
    doc.Parse(json_str.c_str());
    if (doc.HasParseError() || !doc.IsObject()) {
      Log::error("PostProcessing: Failed to parse settings JSON");
      return s;
    }

    auto get_bool = [&](const char* key, bool& out) {
      if (doc.HasMember(key) && doc[key].IsBool()) out = doc[key].GetBool();
      };
    auto get_float = [&](const char* key, float& out) {
      if (doc.HasMember(key) && doc[key].IsNumber()) out = doc[key].GetFloat();
      };
    auto get_int = [&](const char* key, int& out) {
      if (doc.HasMember(key) && doc[key].IsInt()) out = doc[key].GetInt();
      };
    auto get_vec3 = [&](const char* key, glm::vec3& out) {
      if (doc.HasMember(key) && doc[key].IsArray() && doc[key].Size() >= 3) {
        auto arr = doc[key].GetArray();
        out.x = arr[0].GetFloat();
        out.y = arr[1].GetFloat();
        out.z = arr[2].GetFloat();
      }
      };
    auto get_ivec3 = [&](const char* key, glm::ivec3& out) {
      if (doc.HasMember(key) && doc[key].IsArray() && doc[key].Size() >= 3) {
        auto arr = doc[key].GetArray();
        out.x = arr[0].GetInt();
        out.y = arr[1].GetInt();
        out.z = arr[2].GetInt();
      }
      };

    get_bool("enabled", s.enabled);
    get_float("gamma", s.gamma);

    get_bool("bloom_enabled", s.bloom_enabled);
    get_float("bloom_threshold", s.bloom_threshold);
    get_float("bloom_intensity", s.bloom_intensity);
    get_int("bloom_iterations", s.bloom_iterations);
    get_vec3("bloom_tint", s.bloom_tint);
    get_bool("bloom_dirt_enabled", s.bloom_dirt_enabled);
    get_float("bloom_anamorphic", s.bloom_anamorphic);

    get_bool("lut_enabled", s.lut_enabled);
    get_float("lut_intensity", s.lut_intensity);

    get_bool("vignette_enabled", s.vignette_enabled);
    get_float("vignette_intensity", s.vignette_intensity);
    get_float("vignette_radius", s.vignette_radius);
    get_vec3("vignette_color", s.vignette_color);
    get_float("vignette_roundness", s.vignette_roundness);
    get_float("vignette_smoothness", s.vignette_smoothness);

    get_bool("chromatic_aberration_enabled", s.chromatic_aberration_enabled);
    get_float("chromatic_aberration_intensity", s.chromatic_aberration_intensity);
    get_int("chromatic_aberration_samples", s.chromatic_aberration_samples);
    get_bool("chromatic_aberration_edge_only", s.chromatic_aberration_edge_only);

    get_bool("crt_enabled", s.crt_enabled);
    get_float("crt_scanline_intensity", s.crt_scanline_intensity);
    get_float("crt_curvature", s.crt_curvature);
    get_float("crt_scanline_count", s.crt_scanline_count);
    get_bool("crt_subpixel_enabled", s.crt_subpixel_enabled);
    get_bool("crt_glow_enabled", s.crt_glow_enabled);
    get_float("crt_glow_intensity", s.crt_glow_intensity);
    get_float("crt_corner_radius", s.crt_corner_radius);
    get_float("crt_flicker_intensity", s.crt_flicker_intensity);

    get_bool("quantize_enabled", s.quantize_enabled);
    get_int("quantize_levels", s.quantize_levels);
    get_bool("quantize_dither_enabled", s.quantize_dither_enabled);
    get_ivec3("quantize_levels_rgb", s.quantize_levels_rgb);
    get_int("quantize_palette", s.quantize_palette);

    get_bool("film_grain_enabled", s.film_grain_enabled);
    get_float("film_grain_intensity", s.film_grain_intensity);
    get_float("film_grain_size", s.film_grain_size);

    get_bool("tonemapping_enabled", s.tonemapping_enabled);
    get_int("tonemapping_mode", s.tonemapping_mode);
    get_float("tonemapping_exposure", s.tonemapping_exposure);

    get_bool("sharpen_enabled", s.sharpen_enabled);
    get_float("sharpen_intensity", s.sharpen_intensity);

    get_int("effect_variant", s.effect_variant);

    get_bool("msaa_enabled", s.msaa_enabled);
    get_int("msaa_samples", s.msaa_samples);

    return s;
  }

} // namespace Engine::Graphics::PostProcessing
