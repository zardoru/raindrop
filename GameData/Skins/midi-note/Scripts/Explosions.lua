if Game:GetPlayer(0).Channels ~= 4 then
	fallback_require("Scripts/Explosions")
	return
end

Explosions = {}

-- Changeable parameters
Explosions.Duration = 0.15
Explosions.KeyupDuration = 0.1
Explosions.DurationHoldEnd = 0.15

function Explosions:Init()
	local Channels = Game:GetPlayer(0).Channels
	print "Running midi-note explosions script"
	self.Receptors = {}
  self.ReceptorFlash = {}
  self.HoldFlash = {}
	
	self.Time = {1, 1, 1, 1}
	self.GearTime = {1, 1, 1, 1}
  self.FlashTime = {1, 1, 1, 1}
	rotTable = {90, 0, 180, 270}
	
	function Locate(Object, i)
		Object.Centered = 1
		Object.X = self.Noteskin["Key" .. i .. "X"]
		Object.Y = self.Player.JudgmentY
		Object.Width = self.Noteskin["Key" .. i .. "Width"]
		Object.Height = self.Noteskin.NoteHeight
		Object.Rotation = rotTable[i]
	end
	
	for i=1, 4 do
		local Object = Engine:CreateObject()
		Object.Texture = "Receptor.png"
		Locate(Object, i)

		self.Receptors[i] = Object
	end
	
  for i=1, 4 do
		local Object = Engine:CreateObject()
		Object.Texture = "ReceptorFlash.png"
    Object.BlendMode = BlendAdd
		Locate(Object, i)

		self.ReceptorFlash[i] = Object
	end
  
  for i=1, 4 do
		local Object = Engine:CreateObject()
		Object.Texture = "Down Tap Explosion Bright.png"
    Object.BlendMode = BlendAdd
    Object.Alpha = 0
		Locate(Object, i)

		self.HoldFlash[i] = Object
	end
  
	self.Sprites = {}
	for i=1, Channels do
		local Object = Engine:CreateObject()
		Object.Texture = "Down Tap Explosion Dim.png"
    Object.BlendMode = BlendAdd
    Object.Lighten = 1
    Object.LightenFactor = 1
		Locate(Object, i)
		
		Object.Alpha = 0
		self.Sprites[i] = Object
	end
end

librd.make_new(Explosions, Explosions.Init)

function Explosions:Run(Delta)
	for Lane=1, 4 do
		
		self.Time[Lane] = self.Time[Lane] + Delta
		self.GearTime[Lane] = self.GearTime[Lane] + Delta
		self.FlashTime[Lane] = self.FlashTime[Lane] + Delta
    
		if self.Time[Lane] < self.Duration then
			-- local newScale = lerp(Explosions.Time[Lane], 0, Explosions.Duration, 0.5, 3)
			self.Sprites[Lane].Alpha = lerp(self.Time[Lane], 0, Explosions.Duration, 1, 0)
      self.Sprites[Lane].LightenFactor = lerp(self.Time[Lane], 0, Explosions.Duration, 4, 0)
		else
			self.Sprites[Lane].Alpha = 0
		end
		
		if self.GearTime[Lane] < self.KeyupDuration then
			local newScale = lerp(self.GearTime[Lane], 0, self.KeyupDuration, 0.5, 1)
			self.Receptors[Lane]:SetScale(newScale)
			self.ReceptorFlash[Lane]:SetScale(newScale)
		else
			self.Receptors[Lane]:SetScale(1)
      self.ReceptorFlash[Lane]:SetScale(1)
		end
    
    local l = self.HoldFlash[Lane]
    if self.FlashTime[Lane] < self.DurationHoldEnd then
      l.Alpha = self.FlashTime[Lane] / self.DurationHoldEnd
      l.LightenFactor = 4 * (self.FlashTime[Lane] / self.DurationHoldEnd)
    else
      l.Alpha = 0
    end
    
    local Bt = math.floor(self.Player.Beat * 8) % 8
    if Bt == 0 then
      self.ReceptorFlash[Lane].Alpha = 0.4
    else
      self.ReceptorFlash[Lane].Alpha = 0
    end
	end
end

function Explosions:GearKeyEvent(Lane, KeyDown)
	if KeyDown then
		self.GearTime[Lane] = 0
	end
end

function Explosions:OnHit(j, t, Lane, Kind, IsHold, IsHoldRelease)
  if IsHoldRelease == 0 then 
    self.Time[Lane] = 0
  else
    self.FlashTime[Lane] = 0
  end
end
