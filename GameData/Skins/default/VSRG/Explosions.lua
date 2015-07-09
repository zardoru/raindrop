Explosions = {}

-- Changeable parameters
Explosions.HitFramerate = 60
Explosions.HitFrames = 10
Explosions.HitScale = 1
Explosions.HitSheet = "VSRG/explsheet.csv"

Explosions.HoldFramerate = 60
Explosions.HoldFrames = 40
Explosions.HoldScale = 1
Explosions.HoldSheet = "VSRG/holdsheet.csv"

Explosions.KeysSheet = "VSRG/keys.csv"

-- Yeah, yeah. This doesn't belong here.
Explosions.Keys = {}

Explosions.MissShow = 1

function Explosions.HitName (i)
	return "lightingN-" .. i-1 .. ".png"
end

function Explosions.HoldName (i)
	return "lightingL-" .. i-1 .. ".png"
end

-- Internal functions
function ObjectPosition(Obj, Atlas, i, Scale)
	Obj.Image = ("VSRG/"..Atlas.File)
	Obj.Centered = (1)
	Obj.X = Noteskin[Channels]["Key"..i.."X"]
	Obj.Y = JudgmentLineY

	if Upscroll ~= 0 then
		Obj.Rotation = (180)
	end

	Obj.Layer = (28)
	Obj.BlendMode = BlendAdd -- Add
	Obj:SetScale(Scale)
end

function Explosions.Init()

	Explosions.HitImages = {}
	Explosions.HitTargets = {}
	Explosions.HitTime = {}
	Explosions.HitColorize = {}
	Explosions.HitFrameTime = 1/Explosions.HitFramerate
	Explosions.HitAnimationDuration = Explosions.HitFrameTime * Explosions.HitFrames

	Explosions.HoldImages = {}
	Explosions.HoldTargets = {}
	Explosions.HoldTime = {}
	Explosions.HoldFrameTime = 1/Explosions.HoldFramerate
	Explosions.HoldDuration = Explosions.HoldFrameTime * Explosions.HoldFrames
	
	Explosions.KeysUp = {}
	Explosions.KeysDown = {}


	Explosions.HitAtlas = TextureAtlas:new(GetSkinFile(Explosions.HitSheet))
	Explosions.HoldAtlas = TextureAtlas:new(GetSkinFile(Explosions.HoldSheet))
	Explosions.KeyAtlas = TextureAtlas:new(GetSkinFile(Explosions.KeysSheet))

	for i = 1, Explosions.HitFrames do
		Explosions.HitImages[i] = Explosions.HitName(i)
	end

	for i = 1, Explosions.HoldFrames do
		Explosions.HoldImages[i] = Explosions.HoldName(i)
	end

	for i = 1, Channels do
		-- Regular explosions
		Explosions.HitTargets[i] = Engine:CreateObject()

		ObjectPosition(Explosions.HitTargets[i], Explosions.HitAtlas, i, Explosions.HitScale)

		Explosions.HitTime[i] = Explosions.HitFrameTime * Explosions.HitFrames
		Explosions.HitColorize[i] = 0

		-- Hold explosions
		Explosions.HoldTargets[i] = Engine:CreateObject()

		ObjectPosition(Explosions.HoldTargets[i], Explosions.HoldAtlas, i, Explosions.HoldScale)
		Explosions.HoldTime[i] = Explosions.HoldDuration
		
		-- Keys
		Explosions.Keys[i] = Engine:CreateObject()
		local obj = Explosions.Keys[i]
		obj.Centered = 1
		obj.X = Noteskin[Channels]["Key" .. i .. "X"]
		obj.Image = Explosions.KeyAtlas.File
		obj.Layer = 27
		Explosions.KeysUp[i] = Noteskin[Channels]["Key" .. i]
		Explosions.KeysDown[i] = Noteskin[Channels]["Key" .. i .. "Down"]
		
		Explosions.KeyAtlas:SetObjectCrop(obj, Explosions.KeysUp[i])
		
		obj.Width = Noteskin[Channels]["Key" .. i .. "Width"]
		obj.Height = Noteskin[Channels].GearHeight
		
		if Upscroll == 1 then
			obj.Y = JudgmentLineY - obj.Height / 2
		else
			obj.Y = JudgmentLineY + obj.Height / 2
		end
	end
end

function Explosions.GearKeyEvent(i, IsKeyDown)
	i = i + 1
	if IsKeyDown == 1 then
		Explosions.KeyAtlas:SetObjectCrop(Explosions.Keys[i], Explosions.KeysDown[i])
	else 
		Explosions.KeyAtlas:SetObjectCrop(Explosions.Keys[i], Explosions.KeysUp[i])
	end
end

function Explosions.Run(Delta)

	for i = 1, Channels do
		Explosions.HitTime[i] = Explosions.HitTime[i] + Delta

		-- Calculate frame
		Frame = Explosions.HitTime[i] / Explosions.HitFrameTime + 1

		if Frame > Explosions.HitFrames or 
			(Explosions.HitColorize[i] ~= 0 and Explosions.MissShow == 0) then
			Explosions.HitTargets[i].Alpha = (0)
		else
			Explosions.HitTargets[i].Alpha = (1)

			-- Assign size and stuff according to texture atlas
			local Tab = Explosions.HitAtlas.Sprites[Explosions.HitImages[math.floor(Frame)]]

			if not Tab then 
				print (Explosions.HitImages[math.floor(Frame)])
			end
			
			Explosions.HitTargets[i]:SetCropByPixels(Tab.x, Tab.x+Tab.w, Tab.y+Tab.h, Tab.y)
			Explosions.HitTargets[i].Width = Tab.w
			Explosions.HitTargets[i].Height = Tab.h

			-- Colorize the explosion red?
			if Explosions.HitColorize[i] ~= 0 then
				Explosions.HitTargets[i].Red = 1.2
				Explosions.HitTargets[i].Green = 0.2
				Explosions.HitTargets[i].Blue = 0.2
			else
				Explosions.HitTargets[i].Red = 1
				Explosions.HitTargets[i].Green = 1
				Explosions.HitTargets[i].Blue = 1
			end

		end

		Explosions.HoldTime[i] = Explosions.HoldTime[i] + Delta

		-- Update animation loop for holds
		while Explosions.HoldTime[i] > Explosions.HoldDuration do
			Explosions.HoldTime[i] = Explosions.HoldTime[i] - Explosions.HoldDuration
		end

		Frame = Explosions.HoldTime[i] / Explosions.HoldFrameTime + 1

		if HeldKeys[i] == 0 then
			Explosions.HoldTargets[i].Alpha = (0)
		else
			Explosions.HoldTargets[i].Alpha = (1)
			local Tab = Explosions.HoldAtlas.Sprites[Explosions.HoldImages[math.floor(Frame)]]

			Explosions.HoldTargets[i]:SetCropByPixels(Tab.x, Tab.x + Tab.w, Tab.y + Tab.h, Tab.y)
			Explosions.HoldTargets[i].Width = Tab.w
			Explosions.HoldTargets[i].Height = Tab.h

		end

	end
end

function Explosions.Hit(Lane, Kind, IsHold, IsHoldRelease)

	if IsHold ~= 0 then
		Explosions.HoldTime[Lane] = 0
	end

	if IsHold == 0 or IsHoldRelease == 1 then
		Explosions.HitTime[Lane] = 0
		Explosions.HitColorize[Lane] = Kind
	end
end
