#version 330 core
in vec2 texcoord;
in vec4 colorfrag;

uniform vec4 color;
uniform sampler2D tex;

out vec4 FragColor;

void main()
{
    float sdf_value;
    float edge_width;
    float mask_alpha;
    sdf_value = texture(tex, texcoord).r;
    edge_width = fwidth(sdf_value);
    mask_alpha = smoothstep(0.5 - edge_width, 0.5 + edge_width, sdf_value);
    if (mask_alpha <= 0.0)
        discard;
    FragColor = vec4(color.rgb, color.a * mask_alpha) * colorfrag;
}
