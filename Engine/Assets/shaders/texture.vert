#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in float aTexId;

out vec4 v_Color;
out vec2 v_TexCoord;
out float v_TexId;

uniform mat4 u_ViewProjection;

void main() {
    v_Color = aColor;
    v_TexCoord = aTexCoord;
    v_TexId = aTexId;
    gl_Position = u_ViewProjection * vec4(aPos, 1.0);
}