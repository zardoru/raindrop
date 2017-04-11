Judgment = {
	FadeoutTime = 0.5,
	FadeoutDuration = 0.15,
	Tilt = 7, -- in degrees

	ScaleTime = 0.1,
	Scale = 0.21,
	ScaleHit = 1.1,
	ScaleOK = 0.9,
	ScaleMiss = 0.7,
	ScaleExtra = 0.2,

	-- if not nil it overrides default position
	--Position = {},

	Table = {
		"judge-excellent.png",
		"judge-perfect.png",
		"judge-good.png",
		"judge-bad.png",
		"judge-miss.png",
	},

	TimingIndicator = "hiterror.png",
	ShowTimingIndicator = true,
	ScaleLerp = Ease.ElasticSquare(2.5)
}


function Judgment:Init()
  print "Judgment Initializing."
	self.Atlas = TextureAtlas:skin_new("VSRG/judgment.csv")
  
  
  self.defaultX = self.Noteskin.GearWidth / 2 + self.Noteskin.GearStartX
  self.defaultY = ScreenHeight * 0.4
  self.ScaleLerp = self.ScaleLerp or function (x) return x end
  print ("Judgment Default Pos: ", self.defaultX, self.defaultY)
  
	self.Object = ScreenObject {
		Layer = 30,
		Centered = 1,
		Texture = self.Atlas.File,
		Alpha = 1
	}

	self.Transform = Transformation()
	self.Object.ChainTransformation = self.Transform
  
	if self.Position then 
		self.Transform.X = self.Position.x or self.defaultX
		self.Transform.Y = self.Position.y or self.defaultY
	else
		self.Transform.X = self.defaultX
		self.Transform.Y = self.defaultY  
	end

	self.Transform.Width = self.Scale
	self.Transform.Height = self.Scale


  	print ("Judgment Real pos/Texture: ", self.Transform.X, self.Transform.Y, self.Object.Texture)

	self.LastAlternation = 0
	self.Time = self.FadeoutTime + self.FadeoutDuration

	self.IndicatorObject = ScreenObject {
		Texture = ("VSRG/" .. self.TimingIndicator),
		Layer = 24,
		Centered = 1,
		Alpha = 0,
	}
	self.IndicatorObject:SetScale( 1 / self.Scale )

	self.IndicatorObject.ChainTransformation = self.Transform
  --self.Value = 0
end

librd.make_new(Judgment, Judgment.Init)

function Judgment:GetComboLerp()
	local AAAThreshold = 8.0 / 9.0
	return clerp(self.Player.Combo,  -- cur
							 0, self.Player.Scorekeeper.MaxNotes * AAAThreshold, -- start end
							 0, 1) -- start val end val
end

function Judgment:Run(Delta)
	local ComboLerp = self:GetComboLerp()
	
	if Game.Active and self.Value then
		local AlphaRatio
		self.Time = min(self.Time + Delta, self.ScaleTime)

		local sval
		if self.Value < 3 then
			sval = self.ScaleHit
		elseif self.Value < 5 then
			sval = self.ScaleOK
		else
			sval = self.ScaleMiss
		end

		local s = lerp(self.ScaleLerp(self.Time / self.ScaleTime), 0, 1, 1, sval)

		self.Transform.ScaleX = s
		self.Transform.ScaleY = s

		if self.Time > self.FadeoutTime then
			local Time = self.Time - self.FadeoutTime
			if Time > 0 then
				local Ratio = Time / self.FadeoutDuration

				if Ratio < 1 then
					AlphaRatio = 1 - Ratio
				else
					AlphaRatio = 0
				end

			end
		else
			AlphaRatio = 1
		end

		local w = self.Object.Width 
		local h = self.Object.Height 

		self.Object.Alpha = (AlphaRatio)

		if self.LastAlternation == 0 then
			self.Transform.Rotation = (self.Tilt * ComboLerp)
		else
			self.Transform.Rotation = (-self.Tilt * ComboLerp)
		end

		if self.Value ~= 1 and self.ShowTimingIndicator then -- not a "flawless"
			self.IndicatorObject.Alpha = AlphaRatio

			if self.Early then -- early
				self.IndicatorObject.X = - (w / 2 + 130)
			else
				-- late
				self.IndicatorObject.X = w / 2 + 130
			end
		else
			self.IndicatorObject.Alpha = 0
		end
	else
		self.IndicatorObject.Alpha = 0
		self.Object.Alpha = 0
	end
end

function Judgment:OnHit(JudgmentValue, Time, l, h, r, pn)
	if pn ~= self.Player.Number then
		return
	end
	self.Value = JudgmentValue

	if self.Value == 0 then
		self.Object.Lighten = (1)
		self.Object.LightenFactor = (1.5)
		self.Value = 1
	else
		self.Object.Lighten = 0
		self.Object.LightenFactor = 0
	end

	if self.LastAlternation == 0 then
		self.LastAlternation = 1
	else
		self.LastAlternation = 0
	end

	self.Atlas:SetObjectCrop(self.Object, self.Table[self.Value])
	self.Object.Height = self.Atlas.Sprites[self.Table[self.Value]].h
	self.Object.Width = self.Atlas.Sprites[self.Table[self.Value]].w

	if JudgmentValue ~= 5 then
		if JudgmentValue ~= -1 then
			local CLerp = self:GetComboLerp()
			self.Object:SetScale (self.ScaleHit + CLerp * self.ScaleExtra)
		else
			self.Object:SetScale (self.Scale)
		end
	else
		self.Object:SetScale (self.ScaleMiss)
	end

	self.Time = 0
	self.Early = Time < 0
end

function Judgment:OnMiss(t, l, h, pn)
  self:OnHit(5, t, l, h, h, pn)
end
