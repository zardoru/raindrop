game_require "TextureAtlas"
game_require "utils"
game_require "AnimationFunctions"
game_require "Histogram"
skin_require "Global/FadeInScreen"

-- Set up constants for everyone

game_require "noteskin_defs"
skin_require "Scripts/Explosions"
skin_require "Scripts/ComboDisplay"
skin_require "Scripts/KeyLightning"
skin_require "Scripts/FixedObjects"
skin_require "Scripts/ScoreDisplay"
skin_require "Scripts/AutoplayAnimation"
skin_require "Scripts/Lifebar"
skin_require "Scripts/StageAnimation"
skin_require "Scripts/AnimatedObjects"
skin_require "Scripts/Keys"
skin_require "Scripts/Judgment"
skin_require "Scripts/TextDisplay"
skin_require "override"

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
    Keys,
		Judgment,
		Explosions,
		Jambar,
		PlayerText,
    AutoAnimation
	},

	-- Internal functions for automating stuff.
	Init = function ()
    local n = #AnimatedObjects.List
    print (n, "objects in Gameplay Screen")
		AnimatedObjects.Items = {}
		for i = 1, #AnimatedObjects.List do
			if AnimatedObjects.List[i] and AnimatedObjects.List[i].new then
        local p = Game:GetPlayer(0)
        local chan = p.Difficulty.Channels
        local ns
        local TT = p.Turntable and (p.Channels == 6 or p.Channels == 8)
        
        if not TT then
          ns = Noteskin[chan]
        else
          ns = NoteskinSpecial[chan]
        end
        
				AnimatedObjects.Items[i] = AnimatedObjects.List[i]:new({
              Player = p,
              Noteskin = ns
            })
      else
        if not AnimatedObjects.List[i] then
          print ("AnimatedObjects object",i,"is nil")
        end
			end
		end
	end,

	Run = function (Delta)
		for i = 1, #AnimatedObjects.Items do
			if AnimatedObjects.Items[i] and AnimatedObjects.Items[i].Run ~= nil then
				AnimatedObjects.Items[i]:Run(Delta)
			end
		end
	end,

	GearKeyEvent = function (Lane, IsKeyDown, PlayerNumber)
		for i = 1, #AnimatedObjects.List do
			if AnimatedObjects.Items[i] and AnimatedObjects.Items[i].GearKeyEvent ~= nil then
				AnimatedObjects.Items[i]:GearKeyEvent(Lane, IsKeyDown, PlayerNumber)
			end
		end
	end,
  
  OnHit = function (a, b, c, d, e, f)
    for i = 1, #AnimatedObjects.List do
			if AnimatedObjects.Items[i] and AnimatedObjects.Items[i].OnHit ~= nil then
        AnimatedObjects.Items[i]:OnHit(a, b, c, d, e, f)
      end
    end
  end,
  
  OnMiss = function (t, l, h, p)
    for i = 1, #AnimatedObjects.List do
			if AnimatedObjects.Items[i] and AnimatedObjects.Items[i].OnMiss ~= nil then
        AnimatedObjects.Items[i]:OnMiss(t, l, h, p)
      end
    end
  end,
  
  OnActivate = function()
    for i = 1, #AnimatedObjects.List do
			if AnimatedObjects.Items[i] and AnimatedObjects.Items[i].OnActivate ~= nil then
        AnimatedObjects.Items[i]:OnActivate()
      end
    end
  end,
  
  OnSongFinish = function()
    for i = 1, #AnimatedObjects.List do
			if AnimatedObjects.Items[i] and AnimatedObjects.Items[i].OnSongFinish ~= nil then
        AnimatedObjects.Items[i]:OnSongFinish()
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
	AnimatedObjects.OnActivate()
end

function HitEvent(JudgmentValue, TimeOff, Lane, IsHold, IsHoldRelease, PNum)
	-- When hits happen, this function is called.
	AnimatedObjects.OnHit(JudgmentValue, TimeOff, Lane, IsHold, IsHoldRelease, PNum)

	if histogram then
	  	histogram:UpdatePoints()
	end
end

function MissEvent(TimeOff, Lane, IsHold, PNum)
	-- When misses happen, this function is called.
	AnimatedObjects.OnMiss(TimeOff, Lane, IsHold, PNum)

	if histogram then
	  	histogram:UpdatePoints()
	end
end

function KeyEvent(Key, Code, IsMouseInput)
	-- All key events, related or not to gear are handled here
end

function GearKeyEvent (Lane, IsKeyDown, PNum)
	-- Only lane presses/releases are handled here.
	AnimatedObjects.GearKeyEvent(Lane, IsKeyDown, PNum)
end

-- Called when the song is over.
function OnSongFinishedEvent()
	AnimatedObjects.OnSongFinish()
	DoSuccessAnimation()
	return SuccessAnimation.Duration
end

function Update(Delta)
	-- Executed every frame.

	if Game.Active then
		AutoAnimation:Run(Delta)
	end

	AnimatedObjects.Run(Delta)

end
