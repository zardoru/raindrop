
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
			float z = abs(texcoord.y - .5) * 2. / persp;
			gl_FragColor = texture2D(tex, fract(texcoord / z + frac)) * clamp(z, 0, 0.5) * 2. * persp;
		}
	]]

	self.Initialized = true
	self.Blue = Engine:CreateObject()
	self.Pink = Engine:CreateObject()

	self.Pink.Texture  = "Global/pink.png"
	self.Blue.Texture  = "Global/blue.png"

	self.Blue.Shader = self.shader

	self.Pink.Y = -self.Pink.Height
	self.Blue.Y = 0
	self.Pink.Z = 0
	self.Blue.Z = 0

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
