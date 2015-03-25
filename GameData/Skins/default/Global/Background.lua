BackgroundAnimation = {
	Rain1Time = 0,
	Rain2Time = 0,
	RainTotalTime = 1,

	Drops1Time = 0,
	Drops2Time = 0,
	DropsTotalTime = 4
}

function BackgroundAnimation.Init(self)
	self.Drops = Engine:CreateObject()
	self.Drops2 = Engine:CreateObject()
	self.Blue = Engine:CreateObject()
	self.Rain = Engine:CreateObject()
	self.Rain2 = Engine:CreateObject()

	self.Drops2Time = self.DropsTotalTime/2
	self.Rain2Time = self.RainTotalTime/2

	self.Rain.Image = "Global/rainfx.png"
	self.Rain2.Image = "Global/rainfx.png"
	self.Drops.Image = "Global/rainfx2.png"
	self.Drops2.Image = "Global/rainfx2.png"
	self.Blue.Image = "Global/blue.png"

	self.Rain.Z = 0
	self.Rain2.Z = 0
	self.Blue.Z = 0
	self.Drops.Z = 0
	self.Drops2.Z = 0

	self.Drops.BlendMode = BlendAdd
	self.Drops2.BlendMode = BlendAdd
	self.Rain.BlendMode = BlendAdd
	self.Rain2.BlendMode = BlendAdd
	self.Blue.Alpha = 1
end

function BackgroundAnimation.UpdateObjects(self)
	local Frac

	Frac = self.Drops1Time / self.DropsTotalTime
	self.Drops.Y = -self.Drops.Height + self.Drops.Height * Frac * 2
	Frac = self.Drops2Time / self.DropsTotalTime
	self.Drops2.Y = -self.Drops2.Height + self.Drops2.Height * Frac * 2

	Frac = self.Rain1Time / self.RainTotalTime
	self.Rain.Y = -self.Rain.Height + self.Rain.Height * Frac * 2
	Frac = self.Rain2Time / self.RainTotalTime 
	self.Rain2.Y = -self.Rain2.Height + self.Rain2.Height * Frac * 2
end

function BackgroundAnimation.Update(self, Delta)
	self.Drops1Time = self.Drops1Time + Delta
	self.Drops2Time = self.Drops2Time + Delta
	self.Rain1Time = self.Rain1Time + Delta
	self.Rain2Time = self.Rain2Time + Delta

	if self.Drops1Time > self.DropsTotalTime then
		self.Drops1Time = self.Drops1Time - self.DropsTotalTime
	end

	if self.Drops2Time > self.DropsTotalTime then
		self.Drops2Time = self.Drops2Time - self.DropsTotalTime
	end

	if self.Rain1Time > self.RainTotalTime then
		self.Rain1Time = self.Rain1Time - self.RainTotalTime
	end


	if self.Rain2Time > self.RainTotalTime then
		self.Rain2Time = self.Rain2Time - self.RainTotalTime
	end

	self:UpdateObjects()
end
