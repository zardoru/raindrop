#version 330
in vec3 position;
in vec2 vertexUV;
in vec4 colorvert;
uniform mat4 projection;
uniform mat4 mvp;
uniform bool centered;
out vec2 texcoord;
out vec3 Pos_world;
out vec4 colorfrag;

void main()
{
	vec3 k_pos = position;
	if (centered){
		k_pos = k_pos + vec3(-0.5, -0.5, 0);
	}
	gl_Position = projection * mvp * vec4(k_pos.xyz, 1);
	Pos_world = (projection * mvp * vec4(k_pos.xyz, 1)).xyz;
	texcoord = vertexUV;
    colorfrag = colorvert;
}
