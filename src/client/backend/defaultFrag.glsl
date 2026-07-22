#version 330 core
in vec2 texcoord;
in vec4 colorfrag;
uniform vec4 color;
uniform sampler2D tex;

out vec4 FragColor;

void main(void)
{
    FragColor = texture(tex, texcoord) * color * colorfrag;
}
