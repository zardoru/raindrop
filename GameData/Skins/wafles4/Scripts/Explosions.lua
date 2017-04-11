if Game:GetPlayer(0).Channels ~= 4 then
	fallback_require "Scripts/Explosions"
	return
end

Explosions = {}

-- Changeable parameters
Explosions.Duration = 0.15
Explosions.KeyupDuration = 0.1

function Explosions:Init()
	self.Receptors = {}
	
	self.Time = {1, 1, 1, 1}
	self.GearTime = {1, 1, 1, 1}
	self.rotTable = {90, 0, 180, 270}
	print "We're running W4 explosions"
  
	function Locate(Object, i)
		Object.Centered = 1
		Object.X = self.Noteskin["Key" .. i .. "X"]
		Object.Y = self.Player.JudgmentY
		Object.Width = self.Noteskin["Key" .. i .. "Width"]
		Object.Height = self.Noteskin.NoteHeight
		Object.Rotation = self.Noteskin.rotTable[i]
	end
	
	for i=1, 4 do
		local Object = Engine:CreateObject()
		Object.Texture = "Receptor.png"
		Locate(Object, i)

		self.Receptors[i] = Object
	end
	
	self.Sprites = {}
	for i=1, 4 do
		local Object = Engine:CreateObject()
		Object.Texture = "explosion.png"
		Locate(Object, i)
		
		Object.Alpha = 0
    Object.Layer = 18
    Object.BlendMode = BlendAdd
		self.Sprites[i] = Object
	end
end

librd.make_new(Explosions, Explosions.Init)

function Explosions:Run(Delta)
	for Lane=1, 4 do
		
		self.Time[Lane] = self.Time[Lane] + Delta
		self.GearTime[Lane] = self.GearTime[Lane] + Delta
		
		if self.Time[Lane] < self.Duration then
			local newScale = lerp(self.Time[Lane], 0, self.Duration, 0.5, 3)
			self.Sprites[Lane].Alpha = lerp(self.Time[Lane], 0, self.Duration, 1, 0)
			self.Sprites[Lane].ScaleX = newScale
			self.Sprites[Lane].ScaleY = newScale
		else
			self.Sprites[Lane].Alpha = 0
		end
		
		if self.GearTime[Lane] < self.KeyupDuration then
			local newScale = lerp(self.GearTime[Lane], 0, self.KeyupDuration, 0.5, 1)
			self.Receptors[Lane].ScaleX = newScale
			self.Receptors[Lane].ScaleY = newScale
		else
			self.Receptors[Lane]:SetScale(1)
		end
	end
end

function Explosions:GearKeyEvent(Lane, KeyDown)
	if KeyDown then
		self.GearTime[Lane] = 0
	end
end

function Explosions:OnHit(a, b, Lane, Kind, IsHold, IsHoldRelease)
	self.Time[Lane] = 0
end
