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
LoopTotal = 7

ItemTextOffset = {
	X = 15,
	Y = 3
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

-- Auxiliary variables.
NoteImage1 = "VSRG/note1.png"
NoteImage2 = "VSRG/note2.png"
NoteImage3 = "VSRG/note3.png"
NoteImage4 = "VSRG/note4.png"
NoteImage5 = "VSRG/note5.png"
NoteImageHold1 = "VSRG/note1L.png"
NoteImageHold2 = "VSRG/note2L.png"
NoteImageHold3 = "VSRG/note3L.png"
NoteImageHold4 = "VSRG/note4L.png"
NoteImageHold5 = "VSRG/note5L.png"

GearHeightCommon = 135
GearWidth = 336
GearLaneSize = {}
-- GearStartX = (ScreenWidth - GearWidth) / 2
GearStartX = 15

-- Gear height.
GearHeight = GearHeightCommon

-- Barline
BarlineEnabled = 1

-- A value of 0 means from the bottom/top (depending on upscroll) of the note. A value of 1 means the middle of the note.
BarlineOffset = 0
BarlineX = GearStartX

-- This is a default value when it's not found in the ChannelsXX tables.
BarlineWidth = GearWidth

-- Enable or disable the screen filter in 7K.
ScreenFilter = 1

for i = 1, 16 do
	GearLaneSize[i] = GearWidth / i
end

-- Actually-used-by-dotcur-directly variables.

-- show up to 9999 ms off
HitErrorDisplayLimiter = 999
DefaultSpeedUnits = 1000

-- 1 is first after processing SV, 1 is mmod, 2 is cmod, anything else is default.
-- default: first before processing SV
DefaultSpeedKind = -1

-- Note height.
NoteHeight = 16

-- Actual channels configuration.
-- Lane X positions are always centered.
--

C1 = "VSRG/key1.png"
C1D = "VSRG/key1d.png"
C2 = "VSRG/key2.png"
C2D = "VSRG/key2d.png"
C3 = "VSRG/key3.png"
C3D = "VSRG/key3d.png"
C4 = "VSRG/key4.png"
C4D = "VSRG/key4d.png"
C5 = "VSRG/key5.png"
C5D = "VSRG/key5d.png"

-- Channels16 is, of course, DP.
Channels16 = {
    -- Gear bindings
    
    Key1 = C3, -- scratch channel on the left side
    Key2 = C1,
    Key3 = C2,
    Key4 = C1,
    Key5 = C2,
    Key6 = C1,
    Key7 = C2,
    Key8 = C1,
    
    Key9 = C3, -- scratch channel on the right side
    Key10 = C1,
    Key11 = C2,
    Key12 = C1,
    Key13 = C2,
    Key14 = C1,
    Key15 = C2,
    Key16 = C1,


    Key1Down = C3D, -- scratch channel on the left side
    Key2Down = C1D,
    Key3Down = C2D,
    Key4Down = C1D,
    Key5Down = C2D,
    Key6Down = C1D,
    Key7Down = C2D,
    Key8Down = C1D,
    
    Key9Down = C3D, -- scratch channel on the right side
    Key10Down = C1D,
    Key11Down = C2D,
    Key12Down = C1D,
    Key13Down = C2D,
    Key14Down = C1D,
    Key15Down = C2D,
    Key16Down = C1D,

    Key1Binding = 8, -- Use Key8 from config.lua. Notes that follow are keys 1 through 7.
    Key2Binding = 1,
    Key3Binding = 2,
    Key4Binding = 3,
    Key5Binding = 4,
    Key6Binding = 5,
    Key7Binding = 6,
    Key8Binding = 7,
    
    Key9Binding = 16,
    Key10Binding = 9,
    Key11Binding = 10,
    Key12Binding = 11,
    Key13Binding = 12,
    Key14Binding = 13,
    Key15Binding = 14,
    Key16Binding = 15,

    GearHeight = GearHeightCommon,
    BarlineWidth = GearWidth*2,

    -- Note Images
    Key1Image = NoteImage3,
    Key2Image = NoteImage1,
    Key3Image = NoteImage2,
    Key4Image = NoteImage1,
    Key5Image = NoteImage2,
    Key6Image = NoteImage1,
    Key7Image = NoteImage2,
    Key8Image = NoteImage1,
    
    Key9Image = NoteImage3,
    Key10Image = NoteImage1,
    Key11Image = NoteImage2,
    Key12Image = NoteImage1,
    Key13Image = NoteImage2,
    Key14Image = NoteImage1,
    Key15Image = NoteImage2,
    Key16Image = NoteImage1,

    -- Hold Bodies
    Key1HoldImage = NoteImageHold3,
    Key2HoldImage = NoteImageHold1,
    Key3HoldImage = NoteImageHold2,
    Key4HoldImage = NoteImageHold1,
    Key5HoldImage = NoteImageHold2,
    Key6HoldImage = NoteImageHold1,
    Key7HoldImage = NoteImageHold2,
    Key8HoldImage = NoteImageHold1,
    
    Key9HoldImage = NoteImageHold3,
    Key10HoldImage = NoteImageHold1,
    Key11HoldImage = NoteImageHold2,
    Key12HoldImage = NoteImageHold1,
    Key13HoldImage = NoteImageHold2,
    Key14HoldImage = NoteImageHold1,
    Key15HoldImage = NoteImageHold2,
    Key16HoldImage = NoteImageHold1,

    -- Lane positions
    Key1X = GearStartX + GearLaneSize[8] / 2,
    Key2X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8],
    Key3X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 2,
    Key4X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 3,
    Key5X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 4,
    Key6X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 5,
    Key7X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 6,
    Key8X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 7,

    Key9X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 15,
    Key10X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 8,
    Key11X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 9,
    Key12X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 10,
    Key13X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 11,
    Key14X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 12,
    Key15X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 13,
    Key16X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 14,

    -- Lane Widths
    Key1Width = GearLaneSize[8],
    Key2Width = GearLaneSize[8],
    Key3Width = GearLaneSize[8],
    Key4Width = GearLaneSize[8],
    Key5Width = GearLaneSize[8],
    Key6Width = GearLaneSize[8],
    Key7Width = GearLaneSize[8],
    Key8Width = GearLaneSize[8],
    Key9Width = GearLaneSize[8],
    Key10Width = GearLaneSize[8],
    Key11Width = GearLaneSize[8],
    Key12Width = GearLaneSize[8],
    Key13Width = GearLaneSize[8],
    Key14Width = GearLaneSize[8],
    Key15Width = GearLaneSize[8],
    Key16Width = GearLaneSize[8]
}

-- Channels12 is of course, 5k DP.
Channels12 = {
    -- Gear bindings
    Key1 = C3, -- scratch channel on the left side
    Key2 = C1,
    Key3 = C2,
    Key4 = C1,
    Key5 = C2,
    Key6 = C1,

    Key7 = C3, -- scratch channel on the right side
    Key8 = C1,
    Key9 = C2, 
    Key10 =C1,
    Key11 =C2,
    Key12 =C1,

    Key1Down = C3D, -- scratch channel on the left side
    Key2Down = C1D,
    Key3Down = C2D,
    Key4Down = C1D,
    Key5Down = C2D,
    Key6Down = C1D,

    Key7Down = C3D, -- scratch channel on the right side
    Key8Down = C1D,
    Key9Down = C2D, 
    Key10Down =C1D,
    Key11Down =C2D,
    Key12Down =C1D,
    
    Key1Binding = 8, -- Use Key8 from config.lua. Notes that follow are keys 1 through 7.
    Key2Binding = 1,
    Key3Binding = 2,
    Key4Binding = 3,
    Key5Binding = 4,
    Key6Binding = 6,

    Key7Binding = 16,
    Key8Binding = 9,
    Key9Binding = 10,
    Key10Binding = 11,
    Key11Binding = 12,
    Key12Binding = 13,
    GearHeight = GearHeightCommon,
    BarlineWidth = GearWidth*2,

    -- Note Images
    Key1Image = NoteImage3,
    Key2Image = NoteImage1,
    Key3Image = NoteImage2,
    Key4Image = NoteImage1,
    Key5Image = NoteImage2,
    Key6Image = NoteImage1,

    Key7Image = NoteImage3,
    Key8Image = NoteImage1,
    Key9Image = NoteImage2,
    Key10Image = NoteImage1,
    Key11Image = NoteImage2,
    Key12Image = NoteImage1,

    -- Hold Bodies
    Key1HoldImage = NoteImageHold3,
    Key2HoldImage = NoteImageHold1,
    Key3HoldImage = NoteImageHold2,
    Key4HoldImage = NoteImageHold1,
    Key5HoldImage = NoteImageHold2,
    Key6HoldImage = NoteImageHold1,

    Key7HoldImage = NoteImageHold3,
    Key8HoldImage = NoteImageHold1,
    Key9HoldImage = NoteImageHold2,
    Key10HoldImage = NoteImageHold1,
    Key11HoldImage = NoteImageHold2,
    Key12HoldImage = NoteImageHold1,

    -- Lane positions
    Key1X = GearStartX + GearLaneSize[6] / 2,
    Key2X = GearStartX + GearLaneSize[6] / 2 + GearLaneSize[6],
    Key3X = GearStartX + GearLaneSize[6] / 2 + GearLaneSize[6] * 2,
    Key4X = GearStartX + GearLaneSize[6] / 2 + GearLaneSize[6] * 3,
    Key5X = GearStartX + GearLaneSize[6] / 2 + GearLaneSize[6] * 4,
    Key6X = GearStartX + GearLaneSize[6] / 2 + GearLaneSize[6] * 5,

    Key7X = GearStartX + GearLaneSize[6] / 2 + GearLaneSize[6] * 6,
    Key8X = GearStartX + GearLaneSize[6] / 2 + GearLaneSize[6] * 7,
    Key9X = GearStartX + GearLaneSize[6] / 2 + GearLaneSize[6] * 8,
    Key10X = GearStartX + GearLaneSize[6] / 2 + GearLaneSize[6] * 9,
    Key11X = GearStartX + GearLaneSize[6] / 2 + GearLaneSize[6] * 10,
    Key12X = GearStartX + GearLaneSize[6] / 2 + GearLaneSize[6] * 11,

    -- Lane Widths
    Key1Width = GearLaneSize[6],
    Key2Width = GearLaneSize[6],
    Key3Width = GearLaneSize[6],
    Key4Width = GearLaneSize[6],
    Key5Width = GearLaneSize[6],
    Key6Width = GearLaneSize[6],
    Key7Width = GearLaneSize[6],
    Key8Width = GearLaneSize[6],
    Key9Width = GearLaneSize[6],
    Key10Width = GearLaneSize[6],
    Key11Width = GearLaneSize[6],
    Key12Width = GearLaneSize[6],
    Key13Width = GearLaneSize[6],
    Key14Width = GearLaneSize[6],
    Key15Width = GearLaneSize[6],
    Key16Width = GearLaneSize[6],

}
-- Channels9 is, by default, pop'n like.
Channels9 = {
    Key1 = C4,
    Key2 = C1,
    Key3 = C2,
    Key4 = C3,
    Key5 = C5,
    Key6 = C3,
    Key7 = C2,
    Key8 = C1,
    Key9 = C4,
    Key1Down = C4D,
    Key2Down = C1D,
    Key3Down = C2D,
    Key4Down = C3D,
    Key5Down = C5D,
    Key6Down = C3D,
    Key7Down = C2D,
    Key8Down = C1D,
    Key9Down = C4D,
    Key1Binding = 8, -- Use Key8 from config.lua. Notes that 
    Key2Binding = 1,
    Key3Binding = 2,
    Key4Binding = 3,
    Key5Binding = 4,
    Key6Binding = 5,
    Key7Binding = 6,
    Key8Binding = 7,
    Key9Binding = 9,

GearHeight = GearHeightCommon,

    -- Note Images
    Key1Image = NoteImage4,
    Key2Image = NoteImage1,
    Key3Image = NoteImage2,
    Key4Image = NoteImage3,
    Key5Image = NoteImage5,
    Key6Image = NoteImage3,
    Key7Image = NoteImage2,
    Key8Image = NoteImage1,
    Key9Image = NoteImage4,

    -- Hold Bodies
    Key1HoldImage = NoteImageHold4,
    Key2HoldImage = NoteImageHold1,
    Key3HoldImage = NoteImageHold2,
    Key4HoldImage = NoteImageHold3,
    Key5HoldImage = NoteImageHold5,
    Key6HoldImage = NoteImageHold3,
    Key7HoldImage = NoteImageHold2,
    Key8HoldImage = NoteImageHold1,
    Key9HoldImage = NoteImageHold4,

    -- Lane positions
    Key1X = GearStartX + GearLaneSize[9] / 2,
    Key2X = GearStartX + GearLaneSize[9] / 2 + GearLaneSize[9],
    Key3X = GearStartX + GearLaneSize[9] / 2 + GearLaneSize[9] * 2,
    Key4X = GearStartX + GearLaneSize[9] / 2 + GearLaneSize[9] * 3,
    Key5X = GearStartX + GearLaneSize[9] / 2 + GearLaneSize[9] * 4,
    Key6X = GearStartX + GearLaneSize[9] / 2 + GearLaneSize[9] * 5,
    Key7X = GearStartX + GearLaneSize[9] / 2 + GearLaneSize[9] * 6,
    Key8X = GearStartX + GearLaneSize[9] / 2 + GearLaneSize[9] * 7,
    Key9X = GearStartX + GearLaneSize[9] / 2 + GearLaneSize[9] * 8,
    -- Lane Widths
    Key1Width = GearLaneSize[9],
    Key2Width = GearLaneSize[9],
    Key3Width = GearLaneSize[9],
    Key4Width = GearLaneSize[9],
    Key5Width = GearLaneSize[9],
    Key6Width = GearLaneSize[9],
    Key7Width = GearLaneSize[9],
    Key8Width = GearLaneSize[9],
    Key9Width = GearLaneSize[9]

}


-- Channels8 is, by default, 7k+1. Key1 is always the scratch channel.
Channels8 = {
    -- Gear bindings
    Key1 = C3,
    Key2 = C1,
    Key3 = C2,
    Key4 = C1,
    Key5 = C2,
    Key6 = C1,
    Key7 = C2,
    Key8 = C1,
    Key1Down = C3D,
    Key2Down = C1D,
    Key3Down = C2D,
    Key4Down = C1D,
    Key5Down = C2D,
    Key6Down = C1D,
    Key7Down = C2D,
    Key8Down = C1D,
    Key1Binding = 8, -- Use Key8 from config.lua. Notes that follow are keys 1 through 7.
    Key2Binding = 1,
    Key3Binding = 2,
    Key4Binding = 3,
    Key5Binding = 4,
    Key6Binding = 5,
    Key7Binding = 6,
    Key8Binding = 7,

    GearHeight = GearHeightCommon,

    -- Note Images
    Key1Image = NoteImage3,
    Key2Image = NoteImage1,
    Key3Image = NoteImage2,
    Key4Image = NoteImage1,
    Key5Image = NoteImage2,
    Key6Image = NoteImage1,
    Key7Image = NoteImage2,
    Key8Image = NoteImage1,

    -- Hold Bodies
    Key1HoldImage = NoteImageHold3,
    Key2HoldImage = NoteImageHold1,
    Key3HoldImage = NoteImageHold2,
    Key4HoldImage = NoteImageHold1,
    Key5HoldImage = NoteImageHold2,
    Key6HoldImage = NoteImageHold1,
    Key7HoldImage = NoteImageHold2,
    Key8HoldImage = NoteImageHold1,

    -- Lane positions
    Key1X = GearStartX + GearLaneSize[8] / 2,
    Key2X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8],
    Key3X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 2,
    Key4X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 3,
    Key5X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 4,
    Key6X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 5,
    Key7X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 6,
    Key8X = GearStartX + GearLaneSize[8] / 2 + GearLaneSize[8] * 7,

    -- Lane Widths
    Key1Width = GearLaneSize[8],
    Key2Width = GearLaneSize[8],
    Key3Width = GearLaneSize[8],
    Key4Width = GearLaneSize[8],
    Key5Width = GearLaneSize[8],
    Key6Width = GearLaneSize[8],
    Key7Width = GearLaneSize[8],
    Key8Width = GearLaneSize[8]
}

-- 7 Channels. By default, it's o2jam-like.
Channels7 = {
    -- Gear bindings
    Key1 = C4,
    Key2 = C1,
    Key3 = C2,
    Key4 = C3,
    Key5 = C2,
    Key6 = C1,
    Key7 = C4,
    Key1Down = C4D,
    Key2Down = C1D,
    Key3Down = C2D,
    Key4Down = C3D,
    Key5Down = C2D,
    Key6Down = C1D,
    Key7Down = C4D,
    Key1Binding = 1,
    Key2Binding = 2,
    Key3Binding = 3,
    Key4Binding = 4,
    Key5Binding = 5,
    Key6Binding = 6,
    Key7Binding = 7,

    GearHeight = GearHeightCommon,

    -- Note Images
    Key1Image = NoteImage4,
    Key2Image = NoteImage1,
    Key3Image = NoteImage2,
    Key4Image = NoteImage3,
    Key5Image = NoteImage2,
    Key6Image = NoteImage1,
    Key7Image = NoteImage4,

    -- Hold Bodies
    Key1HoldImage = NoteImageHold4,
    Key2HoldImage = NoteImageHold1,
    Key3HoldImage = NoteImageHold2,
    Key4HoldImage = NoteImageHold3,
    Key5HoldImage = NoteImageHold2,
    Key6HoldImage = NoteImageHold1,
    Key7HoldImage = NoteImageHold4,

    -- Lane positions
    Key1X = GearStartX + GearLaneSize[7] / 2,
    Key2X = GearStartX + GearLaneSize[7] / 2 + GearLaneSize[7],
    Key3X = GearStartX + GearLaneSize[7] / 2 + GearLaneSize[7] * 2,
    Key4X = GearStartX + GearLaneSize[7] / 2 + GearLaneSize[7] * 3,
    Key5X = GearStartX + GearLaneSize[7] / 2 + GearLaneSize[7] * 4,
    Key6X = GearStartX + GearLaneSize[7] / 2 + GearLaneSize[7] * 5,
    Key7X = GearStartX + GearLaneSize[7] / 2 + GearLaneSize[7] * 6,

    -- Lane Widths
    Key1Width = GearLaneSize[7],
    Key2Width = GearLaneSize[7],
    Key3Width = GearLaneSize[7],
    Key4Width = GearLaneSize[7],
    Key5Width = GearLaneSize[7],
    Key6Width = GearLaneSize[7],
    Key7Width = GearLaneSize[7]
}

-- 6 Channels. By default, it's BMS-like.
Channels6 = {
    Key1 = C4,
    Key2 = C1,
    Key3 = C2,
    Key4 = C2,
    Key5 = C1,
    Key6 = C4,
    Key1Down = C4D,
    Key2Down = C1D,
    Key3Down = C2D,
    Key4Down = C2D,
    Key5Down = C1D,
    Key6Down = C4D,
    GearHeight = GearHeightCommon,
    Key1Binding = 1,
    Key2Binding = 2,
    Key3Binding = 3,
    Key4Binding = 5,
    Key5Binding = 6,
    Key6Binding = 7,
    Key1Image = NoteImage4,
    Key2Image = NoteImage1,
    Key3Image = NoteImage2,
    Key4Image = NoteImage2,
    Key5Image = NoteImage1,
    Key6Image = NoteImage4,
    Key1HoldImage = NoteImageHold4,
    Key2HoldImage = NoteImageHold1,
    Key3HoldImage = NoteImageHold2,
    Key4HoldImage = NoteImageHold2,
    Key5HoldImage = NoteImageHold1,
    Key6HoldImage = NoteImageHold4,
    Key1Width = GearLaneSize[6],
    Key2Width = GearLaneSize[6],
    Key3Width = GearLaneSize[6],
    Key4Width = GearLaneSize[6],
    Key5Width = GearLaneSize[6],
    Key6Width = GearLaneSize[6],
    Key1X = GearStartX + GearLaneSize[6] / 2,
    Key2X = GearStartX + GearLaneSize[6] / 2 + GearLaneSize[6],
    Key3X = GearStartX + GearLaneSize[6] / 2 + GearLaneSize[6] * 2,
    Key4X = GearStartX + GearLaneSize[6] / 2 + GearLaneSize[6] * 3,
    Key5X = GearStartX + GearLaneSize[6] / 2 + GearLaneSize[6] * 4,
    Key6X = GearStartX + GearLaneSize[6] / 2 + GearLaneSize[6] * 5,
    Key7X = GearStartX + GearLaneSize[6] / 2 + GearLaneSize[6] * 6,
}


-- 5 Channels. By default, ez2dj-like.
Channels5 = {
	Key1 = C1,
	Key2 = C2,
	Key3 = C3,
	Key4 = C2,
	Key5 = C1,
	Key1Down = C1D,
	Key2Down = C2D,
	Key3Down = C3D,
	Key4Down = C2D,
	Key5Down = C1D,
	GearHeight = GearHeightCommon,
	Key1Binding = 2,
	Key2Binding = 3,
	Key3Binding = 4,
	Key4Binding = 5,
	Key5Binding = 6,
	Key1Image = NoteImage1,
	Key2Image = NoteImage2,
	Key3Image = NoteImage3,
	Key4Image = NoteImage2,
	Key5Image = NoteImage1,
	Key1HoldImage = NoteImageHold1,
	Key2HoldImage = NoteImageHold2,
	Key3HoldImage = NoteImageHold3,
	Key4HoldImage = NoteImageHold2,
	Key5HoldImage = NoteImageHold1,
	Key1Width = GearLaneSize[5],
	Key2Width = GearLaneSize[5],
	Key3Width = GearLaneSize[5],
	Key4Width = GearLaneSize[5],
	Key5Width = GearLaneSize[5],
	Key1X = GearStartX + GearLaneSize[5] / 2,
	Key2X = GearStartX + GearLaneSize[5] / 2 + GearLaneSize[5],
	Key3X = GearStartX + GearLaneSize[5] / 2 + GearLaneSize[5] * 2,
	Key4X = GearStartX + GearLaneSize[5] / 2 + GearLaneSize[5] * 3,
	Key5X = GearStartX + GearLaneSize[5] / 2 + GearLaneSize[5] * 4,
}

-- 4 Channels. By default, it's DJMax-like.
Channels4 = {
    Key1 = C1,
    Key2 = C2,
    Key3 = C2,
    Key4 = C1,
    Key1Down = C1D,
    Key2Down = C2D,
    Key3Down = C2D,
    Key4Down = C1D,
    GearHeight = GearHeightCommon,
    Key1Binding = 2,
    Key2Binding = 3,
    Key3Binding = 5,
    Key4Binding = 6,
    Key1Image = NoteImage1,
    Key2Image = NoteImage2,
    Key3Image = NoteImage2,
    Key4Image = NoteImage1,
    Key1HoldImage = NoteImageHold1,
    Key2HoldImage = NoteImageHold2,
    Key3HoldImage = NoteImageHold2,
    Key4HoldImage = NoteImageHold1,
    Key1Width = GearLaneSize[4],
    Key2Width = GearLaneSize[4],
    Key3Width = GearLaneSize[4],
    Key4Width = GearLaneSize[4],
    Key1X = GearStartX + GearLaneSize[4] / 2,
    Key2X = GearStartX + GearLaneSize[4] / 2 + GearLaneSize[4],
    Key3X = GearStartX + GearLaneSize[4] / 2 + GearLaneSize[4] * 2,
    Key4X = GearStartX + GearLaneSize[4] / 2 + GearLaneSize[4] * 3,
}
