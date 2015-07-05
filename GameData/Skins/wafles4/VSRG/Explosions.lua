if Channels ~= 4 then
	fallback_require("Explosions.lua")
	return
end

Explosions = {}

-- Changeable parameters
Explosions.Duration = 0.15
Explosions.KeyupDuration = 0.1

function Explosions.Init()
	Explosions.Receptors = {}
	
	Explosions.Time = {1, 1, 1, 1}
	Explosions.GearTime = {1, 1, 1, 1}
	rotTable = {90, 0, 180, 270}
	
	function Locate(Object, i)
		Object.Centered = 1
		Object.X = Noteskin[Channels]["Key" .. i .. "X"]
		Object.Y = JudgmentLineY
		Object.Width = Noteskin[Channels]["Key" .. i .. "Width"]
		Object.Height = NoteHeight
		Object.Rotation = rotTable[i]
	end
	
	for i=1, 4 do
		local Object = Engine:CreateObject()
		Object.Image = "Receptor.png"
		Locate(Object, i)

		Explosions.Receptors[i] = Object
	end
	
	Explosions.Sprites = {}
	for i=1, Channels do
		local Object = Engine:CreateObject()
		Object.Image = "explosion.png"
		Locate(Object, i)
		
		Object.Alpha = 0
		Explosions.Sprites[i] = Object
	end
end

function Explosions.Run(Delta)
	for Lane=1, 4 do
		
		Explosions.Time[Lane] = Explosions.Time[Lane] + Delta
		Explosions.GearTime[Lane] = Explosions.GearTime[Lane] + Delta
		
		if Explosions.Time[Lane] < Explosions.Duration then
			local newScale = lerp(Explosions.Time[Lane], 0, Explosions.Duration, 0.5, 3)
			Explosions.Sprites[Lane].Alpha = lerp(Explosions.Time[Lane], 0, Explosions.Duration, 1, 0)
			Explosions.Sprites[Lane].ScaleX = newScale
			Explosions.Sprites[Lane].ScaleY = newScale
		else
			Explosions.Sprites[Lane].Alpha = 0
		end
		
		if Explosions.GearTime[Lane] < Explosions.KeyupDuration then
			local newScale = lerp(Explosions.GearTime[Lane], 0, Explosions.KeyupDuration, 0.5, 1)
			Explosions.Receptors[Lane].ScaleX = newScale
			Explosions.Receptors[Lane].ScaleY = newScale
		else
			Explosions.Receptors[Lane]:SetScale(1)
		end
	end
end

function Explosions.GearKeyEvent(Lane, KeyDown)
	if KeyDown == 1 then
		Explosions.GearTime[Lane + 1] = 0
	end
end

function Explosions.Hit(Lane, Kind, IsHold, IsHoldRelease)
	Explosions.Time[Lane] = 0
end
