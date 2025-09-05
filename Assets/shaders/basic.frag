#version 450 core

out vec4 FragColor;

uniform vec4 uColor;

void main() {
    FragColor = vec4(uColor);
}