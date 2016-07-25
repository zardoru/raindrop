-- 7+1 and 5+1 supported only
if Channels ~= 8 and Channels ~= 6 then
	fallback_require "screengameplay7k"
	return
end

game_require "TextureAtlas"
game_require "FixedObjects"

game_require "utils"
game_require "AnimationFunctions"

-- Set up constants for everyone

skin_require "skin_defs"
skin_require "custom_defs"
Components = skin_require "components"

-- All of these will be loaded in the loading screen instead of
-- in the main thread, and will also be unloaded at the end.
skin_require "preload"

-- Status of a lane being pressed or not.
KeyArray = {}

-- Lightning
Lightning = {}
LightingTime = 0.25

Bomb = {}
BombTime = 0.2

function Init()
	FixedObjects.XRatio = SkinScale
	FixedObjects.YRatio = SkinScale
	AutoadjustBackground({
		x = 880 * SkinScale,
		y = 200 * SkinScale,
		w = 2080 * SkinScale,
		h = 1170 * SkinScale
	})

	print("Initializing")
	Components:Init()
	IsFullCombo = false

	print ("Create fixed objects")
	FixedObjects.CreateFromCSV("hare.csv", Noteskin[Channels])
	if Channels == 6 then
		FixedObjects.CreateFromCSV("5k.csv", Noteskin[Channels])
	else
		FixedObjects.CreateFromCSV("7k.csv", Noteskin[Channels])
	end

	for i=1, Channels do 
		KeyArray[i] = 0
	end
end

function Cleanup()
end

function OnFullComboEvent()
end

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
end

function HitEvent(JudgmentValue, TimeOff, Lane, IsHold, IsHoldRelease)
    Components:OnHit(JudgmentValue, TimeOff)
end

function MissEvent(TimeOff, Lane, IsHold)
	Components:OnMiss(5, TimeOff)
end

function KeyEvent(Key, Code, IsMouseInput)
end

function GearKeyEvent (Lane, IsKeyDown)
    KeyArray[Lane + 1] = IsKeyDown
end

-- Called when the song is over.
function OnSongFinishedEvent()
	DoSuccessAnimation()
	return SuccessAnimation.Duration
end

function Update(Delta)
	-- Executed every frame.
	local beatEffect = Beat - math.floor(Beat)

	local SongPercentage = Game:GetSongTime() / (SongDuration + 3)

	if Game:GetSongTime() < 0 then
		SongPercentage = math.pow(SongTime / -1.5, 2)
	end

	Components:Update(Delta)
end
