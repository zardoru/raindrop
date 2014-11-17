game_require("textureatlas.lua")
skin_require("Global/AnimationFunctions.lua")

-- Set up constants for everyone
ChannelSpace = "Channels" .. Channels
GearStartX = GetConfigF("GearStartX", "")
GearWidth = GetConfigF("GearWidth", ChannelSpace)
GearHeight = GetConfigF("GearHeight", "")

if Channels == 16 or Channels == 12 then -- DP styles
	GearWidth = GearWidth * 2
end

skin_require("VSRG/Explosions.lua")
skin_require("VSRG/ComboDisplay.lua")
skin_require("VSRG/KeyLightning.lua")
skin_require("VSRG/FixedObjects.lua")
skin_require("VSRG/AnimatedObjects.lua")
skin_require("VSRG/ScoreDisplay.lua")
skin_require("VSRG/AutoplayAnimation.lua")
skin_require("VSRG/GameplayObjects.lua")
skin_require("VSRG/StageAnimation.lua")
skin_require("VSRG/TextDisplay.lua")


-- All of these will be loaded in the loading screen instead of
-- in the main thread once loading is over.
Preload = {
	"VSRG/judgeline.png",
	"VSRG/filter.png",
	"VSRG/stage-left.png",
	"VSRG/stage-right.png",
	"VSRG/pulse_ver.png",
	"VSRG/stagefailed.png",
	"VSRG/progress_tick.png",
	"VSRG/stage-lifeb.png",
	"VSRG/stage-lifeb-s.png",
	"VSRG/combosheet.png",
	"VSRG/explsheet.png",
	"VSRG/holdsheet.png",
	"VSRG/note1.png",
	"VSRG/note2.png",
	"VSRG/note3.png",
	"VSRG/note1L.png",
	"VSRG/note2L.png",
	"VSRG/note3L.png",
	"VSRG/key1.png",
	"VSRG/key2.png",
	"VSRG/key3.png",
	"VSRG/key1d.png",
	"VSRG/key2d.png",
	"VSRG/key3d.png",
	"VSRG/judge-perfect.png",
	"VSRG/judge-excellent.png",
	"VSRG/judge-good.png",
	"VSRG/judge-bad.png",
	"VSRG/judge-miss.png",
	"VSRG/auto.png"
}

-- A convenience class to handle events and such.
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
		Judgment,
		Explosions
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
	DrawTextObjects()

	Obj.SetTarget(ScreenBackground)
	Obj.SetAlpha(0)
end

function Cleanup()

	-- When exiting the screen, this function is called.
	-- It's important to clean all targets or memory will be leaked.

	AnimatedObjects.Cleanup()
	AutoAnimation.Cleanup()	
end

function OnFullComboEvent()
	IsFullCombo = 1
end

-- Returns duration of failure animation.
function OnFailureEvent()
	DoFailAnimation()
	return FailAnimation.Duration
end

function BackgroundFadeIn(frac)
	Obj.SetAlpha(frac)
	return 1
end

-- When 'enter' is pressed and the game starts, this function is called.
function OnActivateEvent()

	Obj.SetTarget(ScreenBackground)
	Obj.AddAnimation("BackgroundFadeIn", 1, 0, EaseNone)

	if Auto ~= 0 then
		AutoAnimation.Init()
	end
end

function HitEvent(JudgmentValue, TimeOff, Lane, IsHold, IsHoldRelease)
	-- When hits happen, this function is called.
	if math.abs(TimeOff) < AccuracyHitMS then
		DoColor = 0

		if JudgmentValue == 0 then
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
function OnSongFinishedEvent()
	AutoAnimation.Finish()
	DoSuccessAnimation()
	return SuccessAnimation.Duration
end

function Update(Delta)
	-- Executed every frame.
	
	if Active ~= 0 then
		AutoAnimation.Run(Delta)
	end
	
	AnimatedObjects.Run(Delta)
	UpdateTextObjects()

end

