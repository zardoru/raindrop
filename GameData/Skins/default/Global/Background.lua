
if BackgroundAnimation then
	return
end

BackgroundAnimation = {Duration = 0.25}

function BackgroundAnimation.Init(self)
	if self.Initialized then
		return
	end

	self.Initialized = true
	self.Blue = Engine:CreateObject()
	self.Pink = Engine:CreateObject()

	self.Pink.Texture  = "Global/pink.png"
	self.Blue.Texture  = "Global/blue.png"
	self.Pink.AffectedByLightning = 1
	self.Blue.AffectedByLightning = 1

	self.Pink.Y = -self.Pink.Height
	self.Blue.Y = self.Blue.Height
	self.Pink.Z = 0
	self.Blue.Z = 0
end

function BGAOut(frac)
	BackgroundAnimation.Pink.Y = -BackgroundAnimation.Pink.Height * (1-frac)
	BackgroundAnimation.Blue.Y = BackgroundAnimation.Blue.Height * (1-frac)
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

function BackgroundAnimation.Update(self, Delta)
end
