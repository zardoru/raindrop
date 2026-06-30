#version 330
in vec2 texcoord;
in vec3 Pos_world;
in vec4 colorfrag;
uniform vec4 color;
uniform sampler2D tex;
uniform bool inverted;
uniform bool replaceColor;
uniform float hdcenter;
uniform float flsize;
uniform float hdsize;
uniform bool BlackToTransparent; // If true, transform r0 g0 b0 aX to a0.
uniform int HiddenLightning;

out vec4 FragColor;

void main(void)
{
    vec4 tCol;
	 vec4 tex2D;
	 if (!replaceColor){
		tex2D = texture(tex, texcoord);
	 }else{
		float a = texture(tex, texcoord).r;
		float r = 0.5;
		float rw = fwidth(a);
       float s = smoothstep(r - rw, r + rw, a);
       if (s <= 0) discard;
		tex2D = vec4(1.0, 1.0, 1.0, s);
	 }
	 if (inverted) {
		tCol = vec4(1.0, 1.0, 1.0, tex2D.a*2) - tex2D * color;
	 }else{
		tCol = tex2D * color;
	 }
	 if (BlackToTransparent) {
			if (tCol.r == 0 && tCol.g == 0 && tCol.b == 0) tCol.a = 0;
	 }
	if (HiddenLightning > 0) {
       float ld = 1;
		if (HiddenLightning == 2)
           ld = smoothstep(hdcenter - hdsize, hdcenter, Pos_world.y);
		else if (HiddenLightning == 1)
           ld = smoothstep(hdcenter, hdcenter - hdsize, Pos_world.y);
		else { ld = smoothstep(hdcenter - flsize - hdsize / 2, hdcenter - flsize, Pos_world.y) *
                   smoothstep(hdcenter + flsize + hdsize / 2, hdcenter + flsize, Pos_world.y);
 }
		FragColor = vec4(tCol.rgb, tCol.a * ld);
	} else if (HiddenLightning == 0) {
		FragColor = tCol;
	}
    FragColor *= colorfrag;
}
