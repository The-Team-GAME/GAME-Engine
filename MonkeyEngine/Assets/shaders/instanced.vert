#version 330 core

// --- PER-VERTEX DATA (Shared unit quad - only 4 vertices) ---
layout (location = 0) in vec2 a_position;   // Unit quad corners (-0.5 to 0.5)
layout (location = 1) in vec2 a_texCoord;   // Base UVs (0 to 1)

// --- PER-INSTANCE DATA (Unique per object) ---
// A mat4 occupies 4 attribute slots (locations 2, 3, 4, 5)
layout (location = 2) in mat4 a_model;      // Model/transform matrix
layout (location = 6) in vec4 a_color;      // Instance color + alpha
layout (location = 7) in vec2 a_uv_min;     // UV min corner (subtexture)
layout (location = 8) in vec2 a_uv_max;     // UV max corner (subtexture)

// --- UNIFORMS (Camera) ---
uniform mat4 u_projection;
uniform mat4 u_view;

// --- OUTPUTS ---
out vec2 v_texCoord;
out vec4 v_color;

void main()
{
    // 1. Transform unit quad vertex by the instance's model matrix
    vec4 worldPos = a_model * vec4(a_position, 0.0, 1.0);

    // 2. Apply camera (view & projection)
    gl_Position = u_projection * u_view * worldPos;

    // 3. Remap base UVs [0,1] to subtexture UV range [uv_min, uv_max]
    v_texCoord = a_uv_min + a_texCoord * (a_uv_max - a_uv_min);
    v_color = a_color;
}
