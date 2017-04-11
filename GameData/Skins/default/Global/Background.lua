
if BackgroundAnimation then
	return
end

BackgroundAnimation = {Duration = 0.25}

function BackgroundAnimation.Init(self)
	if self.Initialized then
		return
	end

	self.shader = Shader()
	self.shader:Compile [[
		#version 120
		varying vec2 texcoord;
		uniform sampler2D tex;
		uniform vec4 color;
		uniform float frac;
		uniform float persp;
		void main(void) {
			vec2 uv = texcoord - .5;
			float z = sqrt(abs(.5*.5 - uv.x*uv.x - uv.y*uv.y)) / persp;

			vec2 rv = uv;
			uv /= z;
			
			uv.x += frac / 4.;
			/*vec4 col = texture2D(tex, uv) ;
			float v = (col.r + col.g + col.b) / 3.;
			vec4 gs = vec4(v,v,v,1.);
			vec4 nc = vec4(0.48, 0.79, 1., 1.);
			gl_FragColor = gs * persp * z;
			*/
			gl_FragColor = texture2D(tex, uv) * (z + 0.2);
		}
	]]

	self.Initialized = true
	self.Blue = Engine:CreateObject()
	self.Pink = Engine:CreateObject()

	self.Pink.Texture  = "Global/pink.png"
	self.Blue.Texture  = "Global/tile_aqua.png"

	self.Blue.Height = ScreenHeight
	self.Blue.Width = ScreenWidth
	self.Blue.Shader = self.shader

	self.Pink.Y = -self.Pink.Height
	self.Blue.Y = 0
	self.Pink.Z = 0
	self.Blue.Z = 0
	self.Pink.Alpha = 0
	
	self.t = 0
end

function BGAOut(frac)
	BackgroundAnimation.Pink.Y = -BackgroundAnimation.Pink.Height * (1-frac)
	BackgroundAnimation.shader:Send("persp", (frac) * 0.9 + 0.1)
	return 1
end

function BGAIn(frac)
	return BGAOut(1-frac)
end

function BackgroundAnimation:In()
	Engine:AddAnimation(self.Pink, "BGAIn", EaseIn, BackgroundAnimation.Duration, 0)
end

function BackgroundAnimation:Out()
	Engine:AddAnimation(self.Pink, "BGAOut", EaseOut, BackgroundAnimation.Duration, 0)
end

function BackgroundAnimation.UpdateObjects(self)
end

function BackgroundAnimation:Update(Delta)
	self.t = self.t + Delta
	self.shader.Send(self.shader, "frac", self.t)
end
