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
        
        // Check if this is likely a font texture (single channel)
        if (sampled.g == 0.0 && sampled.b == 0.0 && sampled.a == 1.0)
        {
            // Font rendering: use red channel as alpha
            FragColor = vec4(v_Color.rgb, v_Color.a * sampled.r);
        }
        else
        {
            // Regular texture rendering
            FragColor = sampled * v_Color;
        }
    }
    else
    {
        FragColor = v_Color;
    }
}