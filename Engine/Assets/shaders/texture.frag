#version 450 core

out vec4 FragColor;

in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexId;

uniform sampler2D u_Textures[32];

void main() {
    if (v_TexId > -1.0)
    {
        int id = int(v_TexId);
        FragColor = texture(u_Textures[id], v_TexCoord) * v_Color;
    }
    else
    {
        FragColor = v_Color;
    }
}