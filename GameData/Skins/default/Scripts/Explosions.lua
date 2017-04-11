Explosions = {
	HitFramerate = 60,
	HitFrames = 10,
	HitScale = 1,
	HitSheet = "VSRG/explsheet.csv",

	HoldFramerate = 60,
	HoldFrames = 40,
	HoldScale = 1,
	HoldSheet = "VSRG/holdsheet.csv",

	KeysSheet = "VSRG/keys.csv",
	MissShow = 1
}

function Explosions.HitName (i)
	return "lightingN-" .. i-1 .. ".png"
end

function Explosions.HoldName (i)
	return "lightingL-" .. i-1 .. ".png"
end

-- Internal functions
function Explosions:ObjectPosition(Obj, Atlas, i, Scale)
	Obj.Texture = "VSRG/"..Atlas.File
	Obj.Centered = 1
	Obj.X = self.Noteskin["Key"..i.."X"]
	Obj.Y = self.Player.JudgmentY

	if self.Player.Upscroll ~= 0 then
		Obj.Rotation = 180
	end

	Obj.Layer = (28)
	Obj.BlendMode = BlendAdd -- Add
	Obj:SetScale(Scale)
  Obj.Alpha = 0
end

function Explosions:Init()
	self.HitImages = {}
	self.HitTargets = {}
	self.HitTime = {}
	self.HitColorize = {}
	self.HitFrameTime = 1/self.HitFramerate
	self.HitAnimationDuration = self.HitFrameTime * self.HitFrames

	self.HoldImages = {}
	self.HoldTargets = {}
	self.HoldTime = {}
	self.HoldFrameTime = 1/self.HoldFramerate
	self.HoldDuration = self.HoldFrameTime * self.HoldFrames


	self.HitAtlas = TextureAtlas:skin_new(self.HitSheet)
	self.HoldAtlas = TextureAtlas:skin_new(self.HoldSheet)

	for i = 1, self.HitFrames do
		self.HitImages[i] = self.HitName(i)
	end

	for i = 1, self.HoldFrames do
		self.HoldImages[i] = self.HoldName(i)
	end

	for i = 1, self.Player.Channels do
		-- Regular explosions
		self.HitTargets[i] = Engine:CreateObject()

		self:ObjectPosition(self.HitTargets[i], self.HitAtlas, i, self.HitScale)

		self.HitTime[i] = self.HitFrameTime * self.HitFrames
		self.HitColorize[i] = 0

		-- Hold explosions
		self.HoldTargets[i] = Engine:CreateObject()

		self:ObjectPosition(self.HoldTargets[i], self.HoldAtlas, i, self.HoldScale)
		self.HoldTime[i] = self.HoldDuration
	end
end

librd.make_new(Explosions, Explosions.Init)

function Explosions:Run(Delta)
	for i = 1, self.Player.Channels do
		self.HitTime[i] = self.HitTime[i] + Delta

		-- Calculate frame
		Frame = self.HitTime[i] / self.HitFrameTime + 1

		if Frame > self.HitFrames or
			(self.HitColorize[i] ~= 0 and self.MissShow == 0) then
			self.HitTargets[i].Alpha = (0)
		else
			self.HitTargets[i].Alpha = (1)

			-- Assign size and stuff according to texture atlas
			local Tab = self.HitAtlas.Sprites[self.HitImages[floor(Frame)]]

			--[[if not Tab then
				print (self.HitImages[math.floor(Frame)])
			end]]

			self.HitTargets[i]:SetCropByPixels(Tab.x, Tab.x+Tab.w, Tab.y+Tab.h, Tab.y)
			self.HitTargets[i].Width = Tab.w
			self.HitTargets[i].Height = Tab.h

			-- Colorize the explosion red?
			if self.HitColorize[i] then
				self.HitTargets[i].Red = 1.2
				self.HitTargets[i].Green = 0.2
				self.HitTargets[i].Blue = 0.2
			else
				self.HitTargets[i].Red = 1
				self.HitTargets[i].Green = 1
				self.HitTargets[i].Blue = 1
			end

		end

		self.HoldTime[i] = self.HoldTime[i] + Delta

		-- Update animation loop for holds
		while self.HoldTime[i] > self.HoldDuration do
			self.HoldTime[i] = self.HoldTime[i] - self.HoldDuration
		end

		Frame = self.HoldTime[i] / self.HoldFrameTime + 1

		if not self.Player:IsHoldActive(i - 1) then
			self.HoldTargets[i].Alpha = (0)
		else
			self.HoldTargets[i].Alpha = (1)
			local Tab = self.HoldAtlas.Sprites[self.HoldImages[floor(Frame)]]

			self.HoldTargets[i]:SetCropByPixels(Tab.x, Tab.x + Tab.w, Tab.y + Tab.h, Tab.y)
			self.HoldTargets[i].Width = Tab.w
			self.HoldTargets[i].Height = Tab.h

		end

	end
end

function Explosions:OnHit(j, t, l, IsHold, IsHoldRelease, pn)
  if pn ~= self.Player.Number then
    return
  end

	if IsHold then
		self.HoldTime[l] = 0
	end

	if (not IsHold) or IsHoldRelease then
		self.HitTime[l] = 0
		self.HitColorize[l] = false
	end
end

function Explosions:OnMiss(t, l, i, pn)
	if pn ~= self.Player.Number then
    return
  end
  
  if not IsHold then
		self.HitTime[l] = 0
		self.HitColorize[l] = true
	end
end
