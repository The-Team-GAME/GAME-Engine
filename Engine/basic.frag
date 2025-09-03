#version 450 core

out vec4 FragColor;

uniform vec4 uColor;

void main() {
    // FragColor = uColor;
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}