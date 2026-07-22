#version 330 core
in vec2 texcoord;
in vec3 Pos_world;
in vec4 colorfrag;

uniform vec4 color;
uniform sampler2D tex;
uniform float hdcenter;
uniform float flsize;
uniform float hdsize;
uniform int HiddenLightning;

out vec4 FragColor;

void main()
{
    vec4 tinted;
    float visibility;

    tinted = texture(tex, texcoord) * color;
    visibility = 1.0;

    if (HiddenLightning == 2)
        visibility = smoothstep(hdcenter - hdsize, hdcenter, Pos_world.y);
    else if (HiddenLightning == 1)
        visibility = smoothstep(hdcenter, hdcenter - hdsize, Pos_world.y);
    else if (HiddenLightning == 3)
        visibility = smoothstep(hdcenter - flsize - hdsize / 2.0, hdcenter - flsize, Pos_world.y) *
                     smoothstep(hdcenter + flsize + hdsize / 2.0, hdcenter + flsize, Pos_world.y);

    FragColor = vec4(tinted.rgb, tinted.a * visibility) * colorfrag;
}
