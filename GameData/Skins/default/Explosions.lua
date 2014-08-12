Explosions = {}

-- Changeable parameters
Explosions.HitFramerate = 60
Explosions.HitFrames = 10
Explosions.HitSize = { w = 250, h = 250 }

Explosions.HoldFramerate = 60
Explosions.HoldFrames = 40
Explosions.HoldSize = { w = 250, h = 250 }

function ObjectPosition(Atlas, i, Size)
	Obj.SetImageSkin(Atlas.File)
	Obj.SetSize(Size.w, Size.h)
	Obj.SetCentered(1)
	Obj.SetPosition(GetConfigF("Key"..i.."X", ChannelSpace), JudgementLineY)

	if Upscroll ~= 0 then
		Obj.SetRotation(180)
	end

	Obj.SetZ(28)
	Obj.SetBlendMode(0) -- Add
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


	Explosions.HitAtlas = TextureAtlas:new(Obj.GetSkinDirectory() .. "explsheet.csv")
	Explosions.HoldAtlas = TextureAtlas:new(Obj.GetSkinDirectory() .. "holdsheet.csv")

	for i = 1, Explosions.HitFrames do
		Explosions.HitImages[i] = "lightingN-" .. i-1 .. ".png"
	end

	for i = 1, Explosions.HoldFrames do
		Explosions.HoldImages[i] = "lightingL-" .. i-1 .. ".png"
	end

	for i = 1, Channels do
		-- Regular explosions
		Explosions.HitTargets[i] = Obj.CreateTarget()

		Obj.SetTarget(Explosions.HitTargets[i])
		ObjectPosition(Explosions.HitAtlas, i, Explosions.HitSize)

		Explosions.HitTime[i] = Explosions.HitFrameTime * Explosions.HitFrames
		Explosions.HitColorize[i] = 0

		-- Hold explosions
		Explosions.HoldTargets[i] = Obj.CreateTarget()

		Obj.SetTarget(Explosions.HoldTargets[i])
		ObjectPosition(Explosions.HoldAtlas, i, Explosions.HoldSize)
		Explosions.HoldTime[i] = Explosions.HoldDuration
	end
end

function Explosions.Cleanup()
	for i = 1, Channels do
		Obj.CleanTarget(Explosions.HitTargets[i])
		Obj.CleanTarget(Explosions.HoldTargets[i])
	end
end

function Explosions.Run(Delta)

	for i = 1, Channels do
		Obj.SetTarget(Explosions.HitTargets[i])
		Explosions.HitTime[i] = Explosions.HitTime[i] + Delta

		-- Calculate frame
		Frame = Explosions.HitTime[i] / Explosions.HitFrameTime + 1

		if Frame > Explosions.HitFrames then
			Obj.SetAlpha(0)
		else
			Obj.SetAlpha(1)

			-- Assign size and stuff according to texture atlas
			local Tab = Explosions.HitAtlas.Sprites[Explosions.HitImages[math.floor(Frame)]]

			Obj.CropByPixels(Tab.x, Tab.y, Tab.x+Tab.w, Tab.y+Tab.h)
			Obj.SetSize(Tab.w, Tab.h)

			-- Colorize the explosion red?
			if Explosions.HitColorize[i] ~= 0 then
				Obj.SetColor(1.2, 0.2, 0.2)
			else
				Obj.SetColor(1, 1, 1)
			end

		end

		Obj.SetTarget(Explosions.HoldTargets[i])

		Explosions.HoldTime[i] = Explosions.HoldTime[i] + Delta

		-- Update animation loop for holds
		while Explosions.HoldTime[i] > Explosions.HoldDuration do
			Explosions.HoldTime[i] = Explosions.HoldTime[i] - Explosions.HoldDuration
		end

		Frame = Explosions.HoldTime[i] / Explosions.HoldFrameTime + 1

		if HeldKeys[i] == 0 then
			Obj.SetAlpha(0)
		else
			Obj.SetAlpha(1)
			local Tab = Explosions.HoldAtlas.Sprites[Explosions.HoldImages[math.floor(Frame)]]

			Obj.CropByPixels(Tab.x, Tab.y, Tab.x + Tab.w, Tab.y + Tab.h)
			Obj.SetSize(Tab.w, Tab.h)

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
