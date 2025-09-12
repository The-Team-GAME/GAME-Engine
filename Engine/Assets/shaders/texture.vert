#version 450 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform bool useTextureRegion;
uniform vec4 spriteCoords; // x, y, width, height

void main() {
    gl_Position = projection * view * model * vec4(aPos, 0.0, 1.0);
    if (useTextureRegion)
    {
        TexCoord = spriteCoords.xy + aTexCoord * spriteCoords.zw;
    }
    else
    {
        TexCoord = aTexCoord;
    }
}