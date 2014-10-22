BackgroundAnimation = {
	Rain1Time = 0,
	Rain2Time = 0,
	RainTotalTime = 1,

	Drops1Time = 0,
	Drops2Time = 0,
	DropsTotalTime = 4,

	Fog1Time = 0,
	Fog2Time = 0,
	FogTotalTime = 10
}

function BackgroundAnimation.Init(self)
	self.Drops = Object2D()
	self.Drops2 = Object2D()
	self.Blue = Object2D()
	self.Rain = Object2D()
	self.Rain2 = Object2D()
	self.Fog = Object2D()
	self.Fog2 = Object2D()

	self.Drops2Time = self.DropsTotalTime/2
	self.Rain2Time = self.RainTotalTime/2
	self.Fog2Time = self.FogTotalTime/2


	self.Fog.Image = "fogfx.png"
	self.Rain.Image = "rainfx.png"
	self.Rain2.Image = "rainfx.png"
	self.Fog2.Image = "fogfx.png"
	self.Drops.Image = "rainfx2.png"
	self.Drops2.Image = "rainfx2.png"
	self.Blue.Image = "blue.png"

	self.Rain.Z = 0
	self.Rain2.Z = 0
	self.Fog.Z = 0
	self.Fog2.Z = 0
	self.Blue.Z = 0
	self.Drops.Z = 0
	self.Drops2.Z = 0
	self.Fog.Y = 0
	self.Fog2.Y = 0

	self.Drops.BlendMode = BlendAdd
	self.Drops2.BlendMode = BlendAdd
	self.Rain.BlendMode = BlendAdd
	self.Rain2.BlendMode = BlendAdd
	self.Blue.Alpha = 0.2
	self.Fog.Alpha = 0.2
	self.Fog2.Alpha = 0.2

	 Engine:AddTarget(self.Drops)
	 Engine:AddTarget(self.Drops2)
	Engine:AddTarget(self.Blue)
	-- Engine:AddTarget(self.Rain)
	-- Engine:AddTarget(self.Rain2)
	-- Engine:AddTarget(self.Fog)
	-- Engine:AddTarget(self.Fog2)
end

function BackgroundAnimation.UpdateObjects(self)
	local Frac

	Frac = self.Fog1Time / self.FogTotalTime
	self.Fog.X = -self.Fog.Width + self.Fog.Width * Frac * 2
	Frac = self.Fog2Time / self.FogTotalTime
	self.Fog2.X = -self.Fog2.Width + self.Fog2.Width * Frac * 2

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
	self.Fog1Time = self.Fog1Time + Delta
	self.Fog2Time = self.Fog2Time + Delta
	self.Drops1Time = self.Drops1Time + Delta
	self.Drops2Time = self.Drops2Time + Delta
	self.Rain1Time = self.Rain1Time + Delta
	self.Rain2Time = self.Rain2Time + Delta

	if self.Fog1Time > self.FogTotalTime then
		self.Fog1Time = self.Fog1Time - self.FogTotalTime
	end

	if self.Fog2Time > self.FogTotalTime then
		self.Fog2Time = self.Fog2Time - self.FogTotalTime
	end

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
