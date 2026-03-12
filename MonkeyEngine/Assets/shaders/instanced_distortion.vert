#version 330 core

// --- PER-VERTEX DATA (Shared unit quad) ---
layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_texCoord;

// --- PER-INSTANCE DATA ---
layout (location = 2) in mat4 a_model;
layout (location = 6) in vec4 a_color;
layout (location = 7) in vec2 a_uv_min;
layout (location = 8) in vec2 a_uv_max;

// --- UNIFORMS ---
uniform mat4 u_projection;
uniform mat4 u_view;
uniform float u_time;
uniform float u_intensity;
uniform float u_speed;
uniform float u_mesh_intensity;

out vec2 v_texCoord;
out vec4 v_color;

// Pseudo-random function
float random(vec2 st) {
  return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

// Noise function
float noise(vec2 st) {
  vec2 i = floor(st);
  vec2 f = fract(st);
  
  float a = random(i);
  float b = random(i + vec2(1.0, 0.0));
  float c = random(i + vec2(0.0, 1.0));
  float d = random(i + vec2(1.0, 1.0));
  
  vec2 u = f * f * (3.0 - 2.0 * f);
  
  return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

void main() {
  // Transform vertex by instance model matrix
  vec4 worldPos = a_model * vec4(a_position, 0.0, 1.0);
  vec2 position = worldPos.xy;
  
  // Only apply mesh distortion if mesh_intensity > 0
  if (u_mesh_intensity > 0.0) {
    float glitch_time = u_time * u_speed;
    
    // Horizontal glitch bars - sharp, blocky displacement
    float y_block = floor(position.y * 0.02);
    float bar_noise = random(vec2(y_block, floor(glitch_time * 4.0)));
    float bar_threshold = 0.75;
    float bar_effect = step(bar_threshold, bar_noise);
    float bar_displacement = bar_effect * u_mesh_intensity * 15.0 * (random(vec2(y_block + 0.5, floor(glitch_time * 4.0))) * 2.0 - 1.0);
    position.x += bar_displacement;
    
    // Sudden vertical jumps in chunks
    float x_block = floor(position.x * 0.02);
    float chunk_noise = random(vec2(x_block, floor(glitch_time * 3.0)));
    float chunk_threshold = 0.80;
    float chunk_effect = step(chunk_threshold, chunk_noise);
    float chunk_displacement = chunk_effect * u_mesh_intensity * 10.0 * (random(vec2(x_block + 0.7, floor(glitch_time * 3.0))) * 2.0 - 1.0);
    position.y += chunk_displacement;
    
    // Random individual vertex jitter for extra chaos
    float vertex_noise = random(vec2(position.x * 0.1, position.y * 0.1 + glitch_time * 10.0));
    float vertex_threshold = 0.92;
    float vertex_effect = step(vertex_threshold, vertex_noise);
    position.x += vertex_effect * u_mesh_intensity * 8.0 * (random(vec2(glitch_time * 15.0, position.y)) * 2.0 - 1.0);
    position.y += vertex_effect * u_mesh_intensity * 8.0 * (random(vec2(glitch_time * 15.0, position.x)) * 2.0 - 1.0);
    
    // Screen tear effect - entire horizontal sections shift
    float tear_line = floor(position.y * 0.01);
    float tear_noise = random(vec2(tear_line, floor(glitch_time * 2.0)));
    float tear_threshold = 0.88;
    float tear_effect = step(tear_threshold, tear_noise);
    position.x += tear_effect * u_mesh_intensity * 20.0 * (random(vec2(tear_line + 1.5, floor(glitch_time * 2.0))) * 2.0 - 1.0);
  }
  
  gl_Position = u_projection * u_view * vec4(position, 0.0, 1.0);
  
  // Remap base UVs to subtexture range
  v_texCoord = a_uv_min + a_texCoord * (a_uv_max - a_uv_min);
  v_color = a_color;
}
