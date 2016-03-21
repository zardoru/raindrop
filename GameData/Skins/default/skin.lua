-- Default skin configuration.

-- Backgrounds
DefaultBackground = "Global/MenuBackground.png"
SelectMusicBackground = ""
EvaluationBackground = DefaultBackground
EvaluationBackground7K = DefaultBackground
MainMenuBackground = ""
DefaultGameplayBackground = DefaultBackground
DefaultGameplay7KBackground = DefaultBackground

-- Gameplay

--[[ Considerations.

	Playing field size is 800x600. This won't change.
	ScreenWidth and ScreenHeight are set automatically.
	"Centered" means to use the center of the image instead of the top-left.

]]

Cursor = {
	RotationSpeed = 140,
	Centered = 1,
	Zooming = 1,
	Size = 60
}

Judgment = {
	Rotation = -90,
	X = 40,
	Y = 370,
	Centered = 1
	-- Size unused. Determined from image size.
}

Lifebar = {
	Height = 84, -- Width is automatically calculated for now.
	X = ScreenWidth - 42,
	Y = 384,
	Rotation = 90,
	Centered = 1
}

-- Audio
AudioManifest = {
	Miss = "miss.wav",
	Fail = function()
		return Global.CurrentGaugeType ~= LT_GROOVE and "stage_failed.ogg" or ""
	end,
	ClickPlay = "drop.wav",

	--[[ 
	--  Better than autodetection.
	--	See, if you make a closure, Decision, Hover, then BGM
	--	will be called in that order. 
	--	That means you can choose them in a consistent way.
	--	ala LR2
	--]]
	SongSelectDecision = "select.wav",
	SongSelectHover = "click.wav",
	SongSelectBGM = function()
		return "loop" .. math.random(8) .. ".ogg" 
	end
}

-- 7K mode configuration.
-- Time that the 'miss' layer will be shown on BMS when a miss occurs.
OnMissBGATime = 0.5

-- Set screen filter transparency on 7K.
ScreenFilter = 0.7

-- Whether to display the in-game histogram.
Histogram = 0

-- show up to 999 ms off
HitErrorDisplayLimiter = 999

-- Whether to go to song select inmediately on failure
GoToSongSelectOnFailure = 0

-- 1 is first after processing SV, 1 is mmod, 2 is cmod, anything else is default.
-- default: first speed before processing SV
DefaultSpeedKind = -1
DefaultSpeedUnits = 1000
