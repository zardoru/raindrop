-- Default skin configuration.

-- Backgrounds
DefaultBackground = "Global/MenuBackground.png"
SelectMusicBackground = ""
EvaluationBackground = DefaultBackground
EvaluationBackground7K = DefaultBackground
MainMenuBackground = ""
DefaultGameplayBackground = DefaultBackground
DefaultGameplay7KBackground = DefaultBackground

-- Audio

-- Amount of .ogg BGM loops. this will be calculated automatically in the future.
LoopTotal = 8

WheelFont = {
	Font = "font.ttf",
	Size = 26
}

ItemTextOffset = {
	X = 15,
	Y = -3
}

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

-- 7K mode configuration.


-- Time that the 'miss' layer will be shown on BMS when a miss occurs.
OnMissBGATime = 3

-- Enable or disable the screen filter in 7K.
ScreenFilter = 1

-- show up to 9999 ms off
HitErrorDisplayLimiter = 999
DefaultSpeedUnits = 1000

-- 1 is first after processing SV, 1 is mmod, 2 is cmod, anything else is default.
-- default: first before processing SV
DefaultSpeedKind = -1

