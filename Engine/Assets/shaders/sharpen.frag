#version 330 core
in vec2 v_texCoord;
out vec4 FragColor;

uniform sampler2D u_scene;
uniform float u_sharpen_intensity; // 0 = none, 1 = full CAS

void main() {
  vec2 uv = v_texCoord;
  
  // Contrast-adaptive sharpening (CAS): sample 5-tap cross pattern
  vec3 a = textureOffset(u_scene, uv, ivec2( 0, -1)).rgb; // top
  vec3 b = textureOffset(u_scene, uv, ivec2(-1,  0)).rgb; // left
  vec3 c = texture(u_scene, uv).rgb;                       // center
  vec3 d = textureOffset(u_scene, uv, ivec2( 1,  0)).rgb; // right
  vec3 e = textureOffset(u_scene, uv, ivec2( 0,  1)).rgb; // bottom

  // Local min/max for CAS clamping (prevents ringing)
  vec3 min_val = min(c, min(min(a, b), min(d, e)));
  vec3 max_val = max(c, max(max(a, b), max(d, e)));

  // Unsharp mask: center - average of neighbors
  vec3 neighbors_avg = 0.25 * (a + b + d + e);
  vec3 sharpened = c + (c - neighbors_avg) * u_sharpen_intensity;

  // CAS clamp: restrict to local neighborhood range
  vec3 color = clamp(sharpened, min_val, max_val);

  FragColor = vec4(color, 1.0);
}
