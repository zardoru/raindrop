ComboDisplay = {
	DigitWidth = 50,
	DigitHeight = 50,
	Trasformation = nil, -- Use this to move the combo.
	BumpTime = 0,
	BumpTotalTime = 0.1, -- Time the animation runs
	BumpFactor = 1.3,
	BumpVertically = true,
	BumpHorizontally = true,
	HeightAddition = -20,
	HoldBumpFactor = 1.2,
  AtlasFile = "VSRG/combosheet.csv",

	ExNotify = true, -- Whether to use the PGREAT-FLAWLESS* notification
	ExNotifyImg = "VSRG/combo_bonus.png",
	ExNotifyTime = 0.34,
	ExNotifyExtraBump = 0.5
}

function ComboDisplay:SetName(i)
	return i-1 .. ".png"
end

function ComboDisplay:Init()
  self.X = self.Position and self.Position.x or self.Noteskin.GearStartX + self.Noteskin.GearWidth / 2
  self.Y = self.Position and self.Position.y or 0.3 * ScreenHeight
  
  if self.Player.Upscroll then
    self.Y = ScreenHeight - self.Y
  end

	self.Atlas = TextureAtlas:skin_new(self.AtlasFile)

	self.Images = {}
	self.Targets = {}

	self.BumpMiss = 1
	self.BumpHit = 2
	self.BumpHold = 3
	self.BumpInactive = 0

	self.BumpColor = 0

	self.BumpKind = self.BumpInactive

	self.BumpRealTotalTime = self.BumpTotalTime

	self.Digits = {}
	self.ExNotifyObject = nil

	for i = 1, 6 do -- Drawing targets
		self.Targets[i] = ScreenObject {
			Texture = self.Atlas.File,
			Centered = 1,
			Layer = 24,
			Alpha = 0
		}

		if self.Transformation then
			self.Targets[i].ChainTransformation = self.Transformation
		end
	end

	for i = 1, 10 do -- Digit images
		self.Images[i] = self:SetName(i)
	end

	if self.ExNotify then
		self.ExNotifyObject = ScreenObject {
			Centered = 1,
			Layer = 24,
			Alpha = 0,
			Texture = self.ExNotifyImg
		}

		self.ExNotifyCurTime = 0
	end
end

librd.make_new(ComboDisplay, ComboDisplay.Init)

function ComboDisplay:Update()
	self.Digits = librd.intToDigits(self.Player.Combo)

	-- active digits = #Digits
	local ActDig = #self.Digits + 1
	local Size = {
		w = self.DigitWidth,
		h = self.DigitHeight
	}

	local DisplaySize = ActDig * Size.w / 2
	self.ExNotifyPos = {
		x = DisplaySize + self.X,
		y = - self.DigitHeight / 2 + self.Y
	}

	for i = 1, 6 do
		local NewPosition = {
			x = DisplaySize - i * Size.w + self.X,
			y = self.HeightAddition * (1 - self.BumpTime / self.BumpTotalTime) + self.Y
		}

		if i < ActDig then
      local d = ActDig - i
			self.Atlas:SetObjectCrop(self.Targets[i], self.Images[self.Digits[d]+1])

			with (self.Targets[i], {
				Width = Size.w,
				Height = Size.h,
				Alpha = 1,
				X = NewPosition.x,
				Y = NewPosition.y
		})
		else
			self.Targets[i].Alpha = 0
		end
	end
end

function ComboDisplay:OnHit(j)

	self:Update()
	self.BumpTime = 0
	self.BumpKind = BumpHit
	self.BumpColor = j == 0

	if self.BumpColor then
		self.ExNotifyCurTime = 0
	end

end

function ComboDisplay:OnMiss()

	self:Update()
	self.BumpTime = 0
	self.BumpKind = BumpMiss

end

function ComboDisplay:Run(Delta)
  local Beat = self.Player.Beat
	local Ratio = 1 - fract(Beat)

	-- the +2 at the topright
	if #self.Digits ~= 0 then

		if self.BumpColor then

			self.ExNotifyObject.X = self.ExNotifyPos.x
			self.ExNotifyObject.Y = self.ExNotifyPos.y

			local Factor = 1 + self.ExNotifyExtraBump * Ratio
			self.ExNotifyObject:SetScale(Factor)

		else -- Time only runs if we're not at an "AWESOME" hit.
			self.ExNotifyCurTime = self.ExNotifyCurTime + Delta
		end

		local Ratio = math.max(1 - self.ExNotifyCurTime / self.ExNotifyTime, 0)
		self.ExNotifyObject.Alpha = Ratio

	else
		self.ExNotifyObject.Alpha = 0
	end

	local RebootHT = false
	for i=1,self.Player.Channels do
	   if self.Player:IsHoldActive(i - 1) then
      RebootHT = true
      break
	   end
	end

	local HoldScale = 1

	if RebootHT then
		local BT = Beat * 2
		local Ratio = (BT - math.floor(BT))
		HoldScale = self.HoldBumpFactor - Ratio * (self.HoldBumpFactor - 1)
	end

	local newScaleRatio = 1

	-- the numbers themselves
	if self.BumpKind ~= 0 then
		self.BumpTime = self.BumpTime + Delta

		local Ratio = self.BumpTime / self.BumpRealTotalTime

		if Ratio >= 1 then
			self.BumpKind = self.BumpInactive;
		end

		newScaleRatio = (self.BumpFactor - (Ratio * math.abs(self.BumpFactor - 1))) * HoldScale
	end

	for i= 1, 6 do
		local scaleX = 1
		local scaleY = 1
		local usedScale

		if self.BumpKind ~= self.BumpInactive then
			usedScale = newScaleRatio
		else
			usedScale = HoldScale
		end

		if self.BumpHorizontally then
			scaleX = usedScale
		end

		if self.BumpVertically then
			scaleY = usedScale
		end

		self.Targets[i].ScaleX = scaleX
		self.Targets[i].ScaleY = scaleY

		if self.BumpColor ~= 0 then
			self.Targets[i].Red = 1
			self.Targets[i].Green = 2.5
			self.Targets[i].Blue = 2.5
		else
			self.Targets[i].Red = 1
			self.Targets[i].Green = 1
			self.Targets[i].Blue = 1
		end
	end

	self:Update()
end
