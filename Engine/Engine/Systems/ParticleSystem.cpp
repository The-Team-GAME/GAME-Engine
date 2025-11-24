//-----------------------------------------------------------------------------------
// file:    ParticleSystem.cpp
// project: Monkey Stole My GPA
//
// author:  Ong Zi Ming (100%)
//          Primary author
//
// email:   o.ziming@digipen.edu
//
// brief:   Implementation of the listener-based particle system. 
//          Each particle uses the engine's rect texture; 
//          entities are destroyed as they fade out.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "ParticleSystem.h"
#include <random>
#include <algorithm>

using namespace Engine;
using namespace Systems;

// ---------------- RNG ----------------
namespace {
  std::mt19937& rng() { static thread_local std::mt19937 g{ std::random_device{}() }; return g; }
  float rand01() { static thread_local std::uniform_real_distribution<float> d(0.f, 1.f); return d(rng()); }
  float rand_range(float a, float b) { return a + (b - a) * rand01(); }
}

// ------------- ctor / init -------------
ParticleSystem::ParticleSystem() {
  Engine::EventManager::add_event_listener<SpawnParticlesEvent>(
    [this](const SpawnParticlesEvent& e) {
      if (!initialized_) lazy_init();
      emitter_.position = e.pos;
      spawn_burst(std::max(0, e.count));
      });

  Engine::EventManager::add_event_listener<SetParticleParamsEvent>(
    [this](const SetParticleParamsEvent& e) {
      if (!initialized_) lazy_init();
      if (e.angle_deg)  emitter_.angle_deg = *e.angle_deg;
      if (e.spread_deg) emitter_.spread_deg = *e.spread_deg;
      if (e.rate)       emitter_.rate = *e.rate;
      if (e.playing)    emitter_.playing = *e.playing;
    });

  subscribed_ = true;
}

ParticleSystem::~ParticleSystem() {
  if (subscribed_) { 
    Engine::EventManager::remove_event_listener<SpawnParticlesEvent>();
    Engine::EventManager::remove_event_listener<SetParticleParamsEvent>();
    subscribed_ = false; 
  }
}

// ------------- init -------------
void ParticleSystem::lazy_init() {
  if (initialized_) return;
  initialized_ = true;
  pool_.assign(pool_size_, ParticleSlot{});
}

bool ParticleSystem::ensure_entity(ParticleSlot& s) {
  if (s.entity.is_valid()) return true;

  const auto white = Engine::Resources::get_rect_texture();
  s.entity = Engine::ECS::create_entity()
    .add_component<Components::Tag<"Particle">>()
    .add_component<Components::Transform2D>(emitter_.position.x, emitter_.position.y, 0.f, 3.f, 3.f)
    .add_component<Components::Texture>(white)
    .add_component<Components::Color>(1.f, 1.f, 1.f, 1.f);
  return s.entity.is_valid();
}

// ------------- spawning -------------
void ParticleSystem::spawn_one(ParticleSlot& s) {
  if (!ensure_entity(s)) return;

  // reset transform/color/life
  if (auto* t = s.entity.get_component<Components::Transform2D>()) {
    t->position.x = emitter_.position.x;
    t->position.y = emitter_.position.y;
    t->scale.x = 3.f; t->scale.y = 3.f;
    t->rotation = 0.f;
  }
  if (auto* c = s.entity.get_component<Components::Color>()) {
    c->r = 1.f; c->g = 0.6f; c->b = 0.1f; c->a = 1.f;
  }

  // velocity from cone with randomized speed
  const float ang0 = deg2rad(emitter_.angle_deg);
  const float half = deg2rad(emitter_.spread_deg * 0.5f);
  const float ang = ang0 + rand_range(-half, half);
  const float spd = rand_range(emitter_.speed_min, emitter_.speed_max);
  s.velocity.x = std::cos(ang) * spd;
  s.velocity.y = -std::sin(ang) * spd;   // up is negative Y if screen coords

  s.life = rand_range(emitter_.life_min, emitter_.life_max);
}

void ParticleSystem::spawn_burst(int n) {
  if (pool_.empty()) return;

  int spawned = 0;
  for (auto& s : pool_) {
    if (!s.entity.is_valid() || s.life <= 0.f) {
      spawn_one(s);
      if (++spawned >= n) break;
    }
  }
}

// ------------- simulation -------------
void ParticleSystem::simulate(float dt) {
  for (auto& s : pool_) {
    if (!s.entity.is_valid() || s.life <= 0.f) continue;

    // life
    s.life -= dt;
    if (s.life <= 0.f) {
      if (s.entity.is_valid()) s.entity.destroy();
      s.entity = {};
      s.life = 0.f;
      s.velocity = { 0.f, 0.f };
      continue;
    }

    // forces
    s.velocity.y += emitter_.gravity * dt;
    s.velocity *= std::max(0.f, 1.f - emitter_.drag * dt);

    // integrate
    if (auto* t = s.entity.get_component<Components::Transform2D>()) {
      t->position.x += s.velocity.x * dt;
      t->position.y += s.velocity.y * dt;
      t->scale.x = std::max(0.f, t->scale.x - emitter_.shrink_rate * dt);
      t->scale.y = std::max(0.f, t->scale.y - emitter_.shrink_rate * dt);
    }

    // fade
    bool invisible = false;
    if (auto* c = s.entity.get_component<Components::Color>()) {
      c->a = std::max(0.f, c->a - emitter_.fade_rate * dt);
      invisible = (c->a <= 0.f);
    }

    if (invisible) {
      if (s.entity.is_valid()) s.entity.destroy();
      s.entity = {};
      s.life = 0.f;
      s.velocity = { 0.f, 0.f };
    }
  }
}

// ------------- update -------------
void ParticleSystem::update() {

  if (!initialized_) return;
  const float dt = Engine::Time::get_fixed_dt();

  if (emitter_.playing && emitter_.rate > 0.f) {
    spawn_accum_ += emitter_.rate * dt;
    int n = static_cast<int>(spawn_accum_);
    if (n > 0) { spawn_accum_ -= static_cast<float>(n); spawn_burst(n); }
  }

  // Sim simulation
  simulate(dt);
}