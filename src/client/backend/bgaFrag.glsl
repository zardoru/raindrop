#version 330 core
in vec2 texcoord;
in vec4 colorfrag;

uniform vec4 color;
uniform sampler2D tex;

out vec4 FragColor;

void main()
{
    vec4 tinted;
    tinted = texture(tex, texcoord) * color;
    if (tinted.r == 0.0 && tinted.g == 0.0 && tinted.b == 0.0)
        tinted.a = 0.0;
    FragColor = tinted * colorfrag;
}
