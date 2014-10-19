game_require("textureatlas.lua")

GearStartX = GetConfigF("GearStartX", "")
GearWidth = GetConfigF("GearWidth", "")
GearHeight = GetConfigF("GearHeight", "")
ChannelSpace = "Channels" .. Channels

if Channels == 16 or Channels == 12 then -- DP styles
	GearWidth = GearWidth * 2
end

skin_require("Explosions.lua")
skin_require("ComboDisplay.lua")
skin_require("KeyLightning.lua")
skin_require("FixedObjects.lua")
skin_require("AnimatedObjects.lua")
skin_require("ScoreDisplay.lua")
skin_require("GameplayObjects.lua")

-- All of these will be loaded in the loading screen instead of
-- in the main thread once loading is over.
Preload = {
	"judgeline.png",
	"filter.png",
	"stage-left.png",
	"stage-right.png",
	"pulse_ver.png",
	"progress_tick.png",
	"stage-lifeb.png",
	"stage-lifeb-s.png",
	"combosheet.png",
	"explsheet.png",
	"holdsheet.png",
	"note1.png",
	"note2.png",
	"note3.png",
	"note1L.png",
	"note2L.png",
	"note3L.png",
	"key1.png",
	"key2.png",
	"key3.png",
	"key1d.png",
	"key2d.png",
	"key3d.png",
	"judge-perfect.png",
	"judge-excellent.png",
	"judge-good.png",
	"judge-bad.png",
	"judge-miss.png",
	"auto.png"
}


AnimatedObjects = {

	-- Insert objects in this list.
	List = {
		Filter,
		Pulse,
		JudgeLine,
		StageLines,
		ProgressTick,
		HitLightning,
		ComboDisplay,
		ScoreDisplay,
		Lifebar,
		MissHighlight,
		Explosions,
		Judgment
	},

	-- Internal functions for automating stuff.
	Init = function ()
		for i = 1, #AnimatedObjects.List do
			AnimatedObjects.List[i].Init()
		end
	end,

	Run = function (Delta)
		for i = 1, #AnimatedObjects.List do
			if AnimatedObjects.List[i].Run ~= nil then
				if AnimatedObjects.List[i].Object ~= nil then
					Obj.SetTarget(AnimatedObjects.List[i].Object)
				end

				AnimatedObjects.List[i].Run(Delta)
			end
		end
	end,

	Cleanup = function ()
		for i = 1, #AnimatedObjects.List do
			AnimatedObjects.List[i].Cleanup()
		end
	end
}

BgAlpha = 0

-- You can only call Obj.CreateTarget, LoadImage and LoadSkin during and after Init is called
-- Not on preload time.
function Init()
	AnimatedObjects.Init()

	Obj.SetTarget(ScreenBackground)
	Obj.SetAlpha(0)
end

function Cleanup()

	-- When exiting the screen, this function is called.
	-- It's important to clean all targets or memory will be leaked.

	AnimatedObjects.Cleanup()

	if AutoBN then
		Obj.CleanTarget (AutoBN)
	end
end

function getMoveFunction(sX, sY, eX, eY)
	return function(frac)
		Obj.SetPosition(sX + (eX - sX)*frac, sY + (eY - sY)*frac)
		return 1
	end
end

function getUncropFunction(w, h, iw, ih)
	return function(frac)
		Obj.SetSize(w, h*(1-frac))
		Obj.CropByPixels(0, ih*frac*0.5, iw, (ih - (ih*frac*0.5)))
		return 1
	end
end

-- When 'enter' is pressed and the game starts, this function is called.
function OnActivate()
	print (Auto)
	if Auto ~= 0 then
		AutoBN = Obj.CreateTarget()
		
		BnMoveFunction = getMoveFunction(GearStartX + GearWidth/2, -60, GearStartX + GearWidth/2, 100)
			
		Obj.SetTarget(AutoBN)
		Obj.SetImageSkin("auto.png")

		Obj.SetCentered(1)

		Obj.AddAnimation( "BnMoveFunction", 0.75, 0, EaseOut )

		w, h = Obj.GetSize()
		factor = GearWidth / w * 3/4
		Obj.SetSize(w * factor, h * factor)
		Obj.SetZ(28)
		AutoFinishAnimation = getUncropFunction(w*factor, h*factor, w, h)
		RunAutoAnimation = true
	end
end

function HitEvent(JudgmentValue, TimeOff, Lane, IsHold, IsHoldRelease)
	-- When hits happen, this function is called.
	if math.abs(TimeOff) < AccuracyHitMS then
		DoColor = 0

		if math.abs(TimeOff) <= 6.4 then
			DoColor = 1
		end

		Explosions.Hit(Lane, 0, IsHold, IsHoldRelease)
		ComboDisplay.Hit(DoColor)

		local EarlyOrLate
		if TimeOff < 0 then
			EarlyOrLate = 1
		else	
			EarlyOrLate = 2
		end

		Judgment.Hit(JudgmentValue, EarlyOrLate)
	end

	ScoreDisplay.Update()
end

function MissEvent(TimeOff, Lane, IsHold)
	-- When misses happen, this function is called.
	if math.abs(TimeOff) <= 135 then -- mishit
		Explosions.Hit(Lane, 1, IsHold, 0)
	end

	local EarlyOrLate
	if TimeOff < 0 then
		EarlyOrLate = 1
	else
		EarlyOrLate = 2
	end

	Judgment.Hit(5, EarlyOrLate)

	ScoreDisplay.Update()
	ComboDisplay.Miss()
	MissHighlight.OnMiss(Lane)
end

function KeyEvent(Key, Code, IsMouseInput)
	-- All key events, related or not to gear are handled here
end

function GearKeyEvent (Lane, IsKeyDown)
	-- Only lane presses/releases are handled here.

	if Lane >= Channels then
		return
	end

	HitLightning.LanePress(Lane, IsKeyDown)
end

-- Called when the song is over.
function SongFinishedEvent()
	if AutoBN then
		Obj.SetTarget(AutoBN)
		Obj.AddAnimation ("AutoFinishAnimation", 0.35, 0, EaseOut)
		RunAutoAnimation = false
	end
end

function Update(Delta)
	-- Executed every frame.
	
	if Active ~= 0 then
		Obj.SetTarget(ScreenBackground)
		a = Obj.GetAlpha()

		if a < 1 then
			Obj.SetAlpha (a + Delta)
		else
			Obj.SetAlpha(1)
		end

		if AutoBN and RunAutoAnimation == true then
			local BeatRate = Beat / 2
			local Scale = math.sin( math.pi * 2 * BeatRate  )
			Scale = Scale * Scale * 0.25 + 0.75
			Obj.SetTarget(AutoBN)
			Obj.SetScale(Scale, Scale)
		end
	end
	
	AnimatedObjects.Run(Delta)
end

