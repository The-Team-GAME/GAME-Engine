#version 330 core
in vec2 v_texCoord;
in vec4 v_color;

out vec4 FragColor;

uniform sampler2D u_texture;
uniform float u_time;
uniform float u_intensity;
uniform float u_speed;

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
  vec2 uv = v_texCoord;
  
  // Create time-based glitch effect
  float glitch_time = u_time * u_speed;
  
  // Horizontal glitch bars
  float bar_noise = noise(vec2(uv.y * 10.0, glitch_time * 2.0));
  float bar_threshold = 0.85;
  float bar_effect = step(bar_threshold, bar_noise);
  
  // RGB shift for glitch effect
  float shift_amount = u_intensity * 0.02 * bar_effect;
  float shift_noise = noise(vec2(glitch_time * 5.0, uv.y * 20.0));
  float shift = shift_amount * (shift_noise * 2.0 - 1.0);
  
  // Sample texture with RGB channel offset
  vec4 color_r = texture(u_texture, vec2(uv.x + shift, uv.y));
  vec4 color_g = texture(u_texture, uv);
  vec4 color_b = texture(u_texture, vec2(uv.x - shift, uv.y));
  
  vec4 final_color = vec4(color_r.r, color_g.g, color_b.b, color_g.a);
  
  // Add random block distortions
  float block_noise = noise(vec2(floor(uv.x * 10.0), floor(uv.y * 10.0) + glitch_time * 3.0));
  float block_threshold = 0.92;
  float block_effect = step(block_threshold, block_noise);
  
  // Horizontal displacement for blocks
  float displacement = block_effect * u_intensity * 0.05 * (random(vec2(glitch_time, uv.y)) * 2.0 - 1.0);
  vec2 displaced_uv = vec2(uv.x + displacement, uv.y);
  
  // Mix displaced and normal based on block effect
  vec4 displaced_color = texture(u_texture, displaced_uv);
  final_color = mix(final_color, displaced_color, block_effect);
  
  // Add scan lines
  float scan_line = sin(uv.y * 800.0 + glitch_time * 10.0) * 0.5 + 0.5;
  scan_line = scan_line * 0.05 * u_intensity;
  final_color.rgb += scan_line;
  
  // Random brightness flicker
  float flicker = noise(vec2(glitch_time * 10.0, 0.0)) * 0.1 * u_intensity;
  final_color.rgb += flicker;
  
  // Apply color tint
  FragColor = final_color * v_color;
  
  // Clamp to valid range
  FragColor = clamp(FragColor, 0.0, 1.0);
}
