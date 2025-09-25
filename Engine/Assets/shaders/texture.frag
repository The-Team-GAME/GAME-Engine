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
        vec4 sampled = texture(u_Textures[id], v_TexCoord);
        
        // If it's a single-channel texture, treat as font
        if (sampled.g == 0.0 && sampled.b == 0.0)
        {
            FragColor = vec4(v_Color.rgb, v_Color.a * sampled.r);
        }
        else
        {
            FragColor = sampled * v_Color;
        }
    }
    else
    {
        FragColor = v_Color;
    }
}