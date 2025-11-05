//-----------------------------------------------------------------------------------
// file:    RotationSystem.h
// project: Monkey Stole My GPA
//
// author:   (100%)
//          Primary author
//
// email:   @digipen.edu
//
// brief:   This file defines a system that handles entity rotation by updating
//          rotation angles based on rotation speed and delta time in the ECS
//          framework.
//
// Copyright © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
//-----------------------------------------------------------------------------------
#include "ParticleSystem.h"
#include <random>
#include <algorithm>

using namespace Engine;
using namespace Systems;

// ---------------- RNG ----------------
static std::mt19937& rng() { static thread_local std::mt19937 g{ std::random_device{}() }; return g; }
float ParticleSystem::rand01() { static thread_local std::uniform_real_distribution<float> d(0.0f, 1.0f); return d(rng()); }
float ParticleSystem::rand_range(float a, float b) { return a + (b - a) * rand01(); }

// ------------- ctor / init -------------
ParticleSystem::ParticleSystem() {
  Engine::EventManager::add_event_listener<SpawnParticlesEvent>(
    [this](const SpawnParticlesEvent& e) {
      if (!initialized_) lazy_init();
      emitter_.position = e.pos;
      spawn_burst(std::max(0, e.count));
      });
  subscribed_ = true;
}

ParticleSystem::~ParticleSystem() {
  if (subscribed_) { 
    Engine::EventManager::remove_event_listener<SpawnParticlesEvent>(); 
    subscribed_ = false; 
  }
}

void ParticleSystem::lazy_init() {
  if (initialized_) return;
  initialized_ = true;

  // Assets
  const auto white = Engine::Resources::get_rect_texture();
  const auto font = Engine::Resources::load_font("fonts/Roboto-Regular.ttf", 16);

  // Pool
  pool_.resize(pool_size_);
  free_list_.reserve(pool_size_);
  for (int i = 0; i < pool_size_; ++i) {
    auto e = Engine::ECS::create_entity()
      .add_component<Components::Transform2D>(emitter_.position.x, emitter_.position.y, 0.0f, emitter_.size0, emitter_.size0)
      .add_component<Components::Texture>(white)
      .add_component<Components::Color>(1.f, 1.f, 1.f, 0.f) // hidden when free
      .add_component<Components::Tag<"Particle">>();
    pool_[i].e = e;
    pool_[i].alive = false;
    free_list_.push_back(i);
  }

}

// ------------- update -------------
void ParticleSystem::update() {
  //lazy_init();

  /*
  if (Engine::Input::is_pressed(KeyCode::MouseLeft)) {
    auto m = Engine::Input::get_mouse_position();
    Engine::EventManager::send_event<SpawnParticlesEvent>(Math::Vec2D{ m.x, m.y }, 64);
  }
  if (Engine::Input::is_pressed(KeyCode::Space)) emitter_.playing = !emitter_.playing;

  const float move = 180.0f * dt;
  if (Engine::Input::is_held(KeyCode::Left))  emitter_.position.x -= move;
  if (Engine::Input::is_held(KeyCode::Right)) emitter_.position.x += move;
  if (Engine::Input::is_held(KeyCode::Up))    emitter_.position.y -= move;
  if (Engine::Input::is_held(KeyCode::Down))  emitter_.position.y += move;


  const float dt = Engine::Time::get_fixed_dt();
  // Continuous spawning
  if (emitter_.playing && emitter_.rate > 0.0f) {
    spawn_accum_ += emitter_.rate * dt;         // particles to spawn (fractional)
    int n = static_cast<int>(spawn_accum_);
    if (n > 0) { spawn_accum_ -= static_cast<float>(n); spawn_burst(n); }
  }
  */
  if (!initialized_) return;
  const float dt = Engine::Time::get_fixed_dt();
  // Sim simulation
  simulate(dt);
}

// ------------- spawn / simulate -------------
void ParticleSystem::spawn_burst(int count) {
  for (int i = 0; i < count; ++i) spawn_one();
}

void ParticleSystem::spawn_one() {
  if (free_list_.empty()) return;
  const int idx = free_list_.back();
  free_list_.pop_back();
  Particle& p = pool_[idx];

  // Velocity cone
  const float ang0 = deg2rad(emitter_.angle_deg);
  const float half = deg2rad(emitter_.spread_deg);
  const float ang = ang0 + (rand01() * 2.f - 1.f) * half;
  const float spd = rand_range(emitter_.speed_min, emitter_.speed_max);

  p.alive = true;
  p.life = rand_range(emitter_.life_min, emitter_.life_max);
  p.max_life = p.life;
  p.pos = emitter_.position;
  p.vel = { std::cos(ang) * spd, -std::sin(ang) * spd }; // y-up
  p.rot = rand_range(-10.f, 10.f);
  p.ang_vel = rand_range(-180.f, 180.f);

  p.size0 = emitter_.size0; p.size1 = emitter_.size1;
  p.start = emitter_.color0; p.end = emitter_.color1;
  p.gravity = emitter_.gravity; p.drag = emitter_.drag;

  if (auto* t = p.e.get_component<Components::Transform2D>()) {
    t->position.x = p.pos.x; t->position.y = p.pos.y;
    t->rotation = p.rot;
    t->scale.x = p.size0; t->scale.y = p.size0;
  }
  if (auto* c = p.e.get_component<Components::Color>()) {
    c->r = p.start.r; c->g = p.start.g; c->b = p.start.b; c->a = p.start.a;
  }
}

static inline Components::Color LerpColor(const Components::Color& a, const Components::Color& b, float t) {
  return {
    a.r + (b.r - a.r) * t,
    a.g + (b.g - a.g) * t,
    a.b + (b.b - a.b) * t,
    a.a + (b.a - a.a) * t
  };
}

void ParticleSystem::simulate(float dt) {
  for (int i = 0; i < pool_size_; ++i) {
    Particle& p = pool_[i];
    if (!p.alive) continue;

    // Lifetime
    p.life -= dt;
    if (p.life <= 0.f) {
      p.alive = false;
      if (auto* c = p.e.get_component<Components::Color>()) c->a = 0.f; // hide
      free_list_.push_back(i);
      continue;
    }

    const float t = 1.0f - (p.life / p.max_life); // 0..1

    // Forces: gravity + drag
    p.vel.y += p.gravity * dt;
    p.vel *= std::max(0.f, 1.f - p.drag * dt);

    // Integrate
    p.pos += p.vel * dt;
    p.rot += p.ang_vel * dt;

    // Curves
    const float size = p.size0 + (p.size1 - p.size0) * t;
    const Components::Color col = LerpColor(p.start, p.end, t);

    // Write back to ECS
    if (auto* tr = p.e.get_component<Components::Transform2D>()) {
      tr->position.x = p.pos.x; tr->position.y = p.pos.y;
      tr->rotation = p.rot;
      tr->scale.x = size; tr->scale.y = size;
    }
    if (auto* c = p.e.get_component<Components::Color>()) {
      c->r = col.r; c->g = col.g; c->b = col.b; c->a = col.a;
    }
  }
}
