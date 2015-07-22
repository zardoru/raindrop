if Channels ~= 4 then
	fallback_require("VSRG/Explosions")
	return
end

Explosions = {}

-- Changeable parameters
Explosions.Duration = 0.15
Explosions.KeyupDuration = 0.1
Explosions.DurationHoldEnd = 0.15

function Explosions.Init()
	Explosions.Receptors = {}
  Explosions.ReceptorFlash = {}
  Explosions.HoldFlash = {}
	
	Explosions.Time = {1, 1, 1, 1}
	Explosions.GearTime = {1, 1, 1, 1}
  Explosions.FlashTime = {1, 1, 1, 1}
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
	
  for i=1, 4 do
		local Object = Engine:CreateObject()
		Object.Image = "ReceptorFlash.png"
    Object.BlendMode = BlendAdd
		Locate(Object, i)

		Explosions.ReceptorFlash[i] = Object
	end
  
  for i=1, 4 do
		local Object = Engine:CreateObject()
		Object.Image = "Down Tap Explosion Bright.png"
    Object.BlendMode = BlendAdd
    Object.Alpha = 0
		Locate(Object, i)

		Explosions.HoldFlash[i] = Object
	end
  
	Explosions.Sprites = {}
	for i=1, Channels do
		local Object = Engine:CreateObject()
		Object.Image = "Down Tap Explosion Dim.png"
    Object.BlendMode = BlendAdd
    Object.Lighten = 1
    Object.LightenFactor = 1
		Locate(Object, i)
		
		Object.Alpha = 0
		Explosions.Sprites[i] = Object
	end
end

function Explosions.Run(Delta)
	for Lane=1, 4 do
		
		Explosions.Time[Lane] = Explosions.Time[Lane] + Delta
		Explosions.GearTime[Lane] = Explosions.GearTime[Lane] + Delta
		Explosions.FlashTime[Lane] = Explosions.FlashTime[Lane] + Delta
    
		if Explosions.Time[Lane] < Explosions.Duration then
			-- local newScale = lerp(Explosions.Time[Lane], 0, Explosions.Duration, 0.5, 3)
			Explosions.Sprites[Lane].Alpha = lerp(Explosions.Time[Lane], 0, Explosions.Duration, 1, 0)
      Explosions.Sprites[Lane].LightenFactor = lerp(Explosions.Time[Lane], 0, Explosions.Duration, 4, 0)
		else
			Explosions.Sprites[Lane].Alpha = 0
		end
		
		if Explosions.GearTime[Lane] < Explosions.KeyupDuration then
			local newScale = lerp(Explosions.GearTime[Lane], 0, Explosions.KeyupDuration, 0.5, 1)
			Explosions.Receptors[Lane]:SetScale(newScale)
			Explosions.ReceptorFlash[Lane]:SetScale(newScale)
		else
			Explosions.Receptors[Lane]:SetScale(1)
      Explosions.ReceptorFlash[Lane]:SetScale(1)
		end
    
    local l = Explosions.HoldFlash[Lane]
    if Explosions.FlashTime[Lane] < Explosions.DurationHoldEnd then
      l.Alpha = lerp(Explosions.FlashTime[Lane], 0, Explosions.DurationHoldEnd, 1, 0)
      l.LightenFactor = lerp(Explosions.FlashTime[Lane], 0, Explosions.DurationHoldEnd, 4, 0)
    else
      l.Alpha = 0
    end
    
    local Bt = math.floor(Game:GetCurrentBeat() * 8) % 8
    if Bt == 0 then
      Explosions.ReceptorFlash[Lane].Alpha = 0.4
    else
      Explosions.ReceptorFlash[Lane].Alpha = 0
    end
	end
end

function Explosions.GearKeyEvent(Lane, KeyDown)
	if KeyDown == 1 then
		Explosions.GearTime[Lane + 1] = 0
	end
end

function Explosions.Hit(Lane, Kind, IsHold, IsHoldRelease)
  if IsHoldRelease == 0 then 
    Explosions.Time[Lane] = 0
  else
    Explosions.FlashTime[Lane] = 0
  end
end
