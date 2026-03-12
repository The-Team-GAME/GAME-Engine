#version 330 core
in vec2 v_texCoord;
out vec4 FragColor;

uniform sampler2D u_source;
uniform vec2 u_texel_size;    // 1.0 / source resolution
uniform bool u_first_pass;    // true = apply brightness threshold
uniform float u_threshold;    // bloom brightness threshold

void main() {
  // Kawase downsample: sample 4 corners + center (13-tap filter for quality)
  vec3 a = texture(u_source, v_texCoord + vec2(-2.0, -2.0) * u_texel_size).rgb;
  vec3 b = texture(u_source, v_texCoord + vec2( 0.0, -2.0) * u_texel_size).rgb;
  vec3 c = texture(u_source, v_texCoord + vec2( 2.0, -2.0) * u_texel_size).rgb;

  vec3 d = texture(u_source, v_texCoord + vec2(-1.0, -1.0) * u_texel_size).rgb;
  vec3 e = texture(u_source, v_texCoord).rgb;
  vec3 f = texture(u_source, v_texCoord + vec2( 1.0, -1.0) * u_texel_size).rgb;

  vec3 g = texture(u_source, v_texCoord + vec2(-2.0,  0.0) * u_texel_size).rgb;
  vec3 h = texture(u_source, v_texCoord + vec2( 0.0,  0.0) * u_texel_size).rgb;
  vec3 i = texture(u_source, v_texCoord + vec2( 2.0,  0.0) * u_texel_size).rgb;

  vec3 j = texture(u_source, v_texCoord + vec2(-1.0,  1.0) * u_texel_size).rgb;
  vec3 k = texture(u_source, v_texCoord + vec2( 1.0,  1.0) * u_texel_size).rgb;

  vec3 l = texture(u_source, v_texCoord + vec2(-2.0,  2.0) * u_texel_size).rgb;
  vec3 m = texture(u_source, v_texCoord + vec2( 0.0,  2.0) * u_texel_size).rgb;
  vec3 n = texture(u_source, v_texCoord + vec2( 2.0,  2.0) * u_texel_size).rgb;

  // Weighted average (matches the 13-tap downsampling from Call of Duty presentation)
  vec3 color = e * 0.125;
  color += (d + f + j + k) * 0.125;
  color += (a + c + l + n) * 0.03125;
  color += (b + g + i + m) * 0.0625;
  color += h * 0.125;

  // Apply brightness threshold on first pass only
  if (u_first_pass) {
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness < u_threshold) {
      color = vec3(0.0);
    } else {
      // Soft knee: smooth falloff around threshold
      float knee = u_threshold * 0.7;
      float soft = brightness - u_threshold + knee;
      soft = clamp(soft / (2.0 * knee + 0.00001), 0.0, 1.0);
      soft = soft * soft;
      color *= max(soft, brightness - u_threshold) / max(brightness, 0.00001);
    }
  }

  FragColor = vec4(color, 1.0);
}
