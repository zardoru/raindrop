game_require "TextureAtlas"
game_require "utils"
game_require "AnimationFunctions"
game_require "Histogram"
skin_require "Global/FadeInScreen"

-- Set up constants for everyone

game_require "noteskin_defs"
GearWidth = Noteskin[Channels].GearWidth
GearHeight = GearHeightCommon
if Upscroll ~= 0 then
	UpscrollMod = -1
else
	UpscrollMod = 1
end

skin_require "VSRG/Explosions"
skin_require "VSRG/ComboDisplay"
skin_require "VSRG/KeyLightning"
skin_require "VSRG/FixedObjects"
skin_require "VSRG/ScoreDisplay"
skin_require "VSRG/AutoplayAnimation"
skin_require "VSRG/GameplayObjects"
skin_require "VSRG/StageAnimation"
skin_require "VSRG/AnimatedObjects"
skin_require "VSRG/TextDisplay"

-- All of these will be loaded in the loading screen instead of
-- in the main thread.
Preload = {
	"VSRG/judgeline.png",
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
	"VSRG/jam_bar.png",
	"VSRG/judgment.png",
	"VSRG/auto.png",
	"VSRG/hitlightning.png",
	"VSRG/hiterror.png",
	"VSRG/miss_highlight.png",
	"VSRG/keys.png",
	"VSRG/fullcombo.png",
	"VSRG/stageclear.png"
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
		Explosions,
		Jambar
	},

	-- Internal functions for automating stuff.
	Init = function ()
		AnimatedObjects.Items = {}
		for i = 1, #AnimatedObjects.List do
			if AnimatedObjects.List[i] then
				AnimatedObjects.Items[i] = AnimatedObjects.List[i]:new()
			end
		end
	end,

	Run = function (Delta)
		for i = 1, #AnimatedObjects.Items do
			if AnimatedObjects.Items[i] and AnimatedObjects.Items[i].Run ~= nil then
				AnimatedObjects.Items[i].Run(Delta)
			end
		end
	end,

	GearKeyEvent = function (Lane, IsKeyDown)
		for i = 1, #AnimatedObjects.List do
			if AnimatedObjects.Items[i] and AnimatedObjects.Items[i].GearKeyEvent ~= nil then
				AnimatedObjects.Items[i].GearKeyEvent(Lane, IsKeyDown)
			end
		end
	end
}

BgAlpha = 0

-- You can only call Engine:CreateObject, LoadImage and LoadSkin during and after Init is called
-- Not on preload time.
function Init()
	AutoadjustBackground()
	AnimatedObjects.Init()
	DrawTextObjects()
	ScreenFade.Init()
	ScreenFade.Out(true)


	if GetConfigF("Histogram", "") ~= 0 then
		histogram = Histogram:new()
		histogram:SetPosition(ScreenWidth - 255, ScreenHeight - 100 - ScoreDisplay.DigitHeight)
		histogram:SetColor(30 / 255, 50 / 255, 200 / 255)
		hist_bg = histogram:SetBackground("Global/white.png")
		hist_bg.Red = 0.2
		hist_bg.Green = 0.2
		hist_bg.Blue = 0.2
	end


	-- ScreenBackground.Alpha = (0)
	Engine:Sort()
end

function Cleanup()
	-- When exiting the screen, this function is called.
	-- It's important to clean all targets or memory will be leaked.
end

function OnFullComboEvent()
	IsFullCombo = 1
end

-- Returns duration of failure animation.
function OnFailureEvent()
	if Global.CurrentGaugeType ~= LT_GROOVE then
		DoFailAnimation()
		return FailAnimation.Duration
	else
		FadeToBlack()
		return SuccessAnimation.Duration
	end
end

-- When 'enter' is pressed and the game starts, this function is called.
function OnActivateEvent()
	if Auto ~= 0 then
		AutoAnimation.Init()
	end
end

function HitEvent(JudgmentValue, TimeOff, Lane, IsHold, IsHoldRelease)
	-- When hits happen, this function is called.
	AnimatedObjects.Hit(JudgmentValue, TimeOff, Lane, IsHold, IsHoldRelease)

	if histogram then
	  	histogram:UpdatePoints()
	end
end

function MissEvent(TimeOff, Lane, IsHold)
	-- When misses happen, this function is called.
	AnimatedObjects.MissEvent(TimeOff, Lane, IsHold)

	if histogram then
	  	histogram:UpdatePoints()
	end
end

function KeyEvent(Key, Code, IsMouseInput)
	-- All key events, related or not to gear are handled here
end

function GearKeyEvent (Lane, IsKeyDown)
	-- Only lane presses/releases are handled here.

	if Lane >= Channels then
		return
	end

	AnimatedObjects.GearKeyEvent(Lane, IsKeyDown)
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
