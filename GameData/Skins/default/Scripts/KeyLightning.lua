HitLightning = { Enabled = true }

HitLightning.Image = "VSRG/HitLightning.png"
HitLightning.Height = 250

local function LightFunction () 
  if GetConfigF("DisableHitlightningAnimation", "") == 1 then 
    return 0 
  else 
    return 1 
  end 
end

--[[
  note to self:
  Constructor takes in a player context and a noteskin.
  Player = ...
  Noteskin = Noteskin[Channels]
  
  something like that.
]]

function HitLightning:Init()
  
	if not self.Enabled or (GetConfigF("Hitlightning", "") == 0) then
    print "Hit Lightning disabled."
		self.Enabled = false
		return
	end
  
  print "Hit Lightning enabled."
  
  self.OffTime = {}

  self.Times = {}
  self.Animate = LightFunction()

  self.Pressed = {}
  self.Position = {}
  self.Channels = self.Player.Channels

	for i = 1, self.Channels do
		self.Times[i] = 1
		self.Pressed[i] = 0
	end

	for i=1, self.Channels do
		self[i] = Engine:CreateObject()

		self.OffTime[i] = 1
		
		self[i].Texture = self.Image
		self[i].Centered = 1
		self[i].BlendMode = BlendAdd
		
		self[i].Width = self.Noteskin["Key"..i.."Width"]
		self[i].Height = self.Height

    local h = self.Player.JudgmentY
		local scrollY = 0
		local scrollX = 0
		scrollX = self.Noteskin["Key" .. i .. "X"]
    
		if self.Player.Upscroll then
			scrollY = h + self.Height / 2
			self[i].Rotation = 180
		else
			scrollY = h - self.Height / 2
		end

		self.Position[i] = { x = scrollX, y = scrollY }
		self[i].X = scrollX
		self[i].Y = scrollY
		self[i].Layer = 15
		self[i].Alpha = 0
	end
end

librd.make_new(HitLightning, HitLightning.Init)

function HitLightning:GearKeyEvent(Lane, IsKeyDown, pn)
	if not self.Enabled or pn ~= self.Player.Number then
		return
	end
  
  local spb = 60 / self.Player.BPM

	if spb ~= math.huge then
		self.OffTime[Lane] = math.min(spb / 1.5, 1)
	end

	if self.OffTime[Lane] > 3 then
		self.OffTime[Lane] = 3
	end	

	if not IsKeyDown then
		self.Times[Lane] = 0
		self.Pressed[Lane] = 0
	else
		self.Pressed[Lane] = 1
	end
end

function HitLightning:Run(Delta)
	if not self.Enabled then
		return
	end

	for i=1, self.Channels do
		self.Times[i] = self.Times[i] + Delta

		if self.Pressed[i] == 0 then
			if self.Times[i] <= self.OffTime[i] then
				if self.Animate == 1 then
					local Lerping = math.pow(self.Times[i] / self.OffTime[i], 2)
					local Additive
					self[i].ScaleX = 1 - Lerping
					self[i].ScaleY = 1 + 1.5 * Lerping

					Additive = self.Height / 2 * 1.5 * Lerping

					if self.Player.Upscroll then
						Additive = Additive * -1
					end

					self[i].Y = self.Position[i].y - Additive
					self[i].Alpha = ( 1 - Lerping )
				elseif self.Animate == 2 then
					local Lerping = math.pow(self.Times[i] / self.OffTime[i], 2)
					local Additive = 0

					Additive = self.Height / 2 * Lerping
					self[i].ScaleY = 1 - Lerping
					self[i].Y = self.Position[i].y + Additive
					self[i].Alpha = 1
				end
			else
				self[i].Alpha = 0
			end

		else
			self[i]:SetScale(1)
			self[i].Alpha = 1
			self[i].Y = self.Position[i].y
		end
	end
end