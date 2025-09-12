#version 450 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D ourTexture;
uniform vec4 uColor;
uniform bool useTexture;

void main() {
    if (useTexture)
    {
        FragColor = texture(ourTexture, TexCoord);
    }
    else
    {
        FragColor = uColor;
    }
}