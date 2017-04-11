local Channels = Game and Game:GetPlayer(0).Channels

if not Channels or (Channels ~= 8 and Channels ~= 6) then
	if Lanes ~= 8 and Lanes ~= 6 then
		print ("Not overwriting noteskin defs - ", Channels, Lanes)
		return
	end
end

print "Running arctichare noteskin defs."

require "utils"
skin_require "skin_defs"

NoteHeight = 10
GearStartX5KP1 = 120
GearStartX7KP1 = 85
GearStartX5KP2 = 1063
GearStartX7KP2 = 3000

Channels6Sizes = {
	200,
	100,
	100,
	100,
	100,
	100
}

Channels8Sizes = {
	200,
	100,
	100,
	100,
	100,
	100,
	100,
	100
}

-- Scale to raindrop size
ScaleMap(Channels6Sizes)
ScaleMap(Channels8Sizes)

-- ws: lane half-width
-- ss: scratch half-width
local ws = 100 * SkinScale / 2
local ss = 200 * SkinScale / 2

-- 6 channels, scratch at the right
C6RelativeRS = {
	ws * 6 + ss,
	ws,
	ws * 2,
	ws * 3,
	ws * 4,
	ws * 5
}

-- 8 channels, scratch at the right
C8RelativeRS = {
	ws * 8 + ss,
	ws,
	ws * 2,
	ws * 3,
	ws * 4,
	ws * 5,
	ws * 6,
	ws * 7
}

-- 6 channels, scratch at the left
C6RelativeLS = {
	ss,
	ss * 2 + ws,
	ss * 2 + ws * 2,
	ss * 2 + ws * 3,
	ss * 2 + ws * 4,
	ss * 2 + ws * 5,
}

-- 8 channels, scratch at the left
C8RelativeLS = {
	ss,
	ss * 2 + ws,
	ss * 2 + ws * 2,
	ss * 2 + ws * 3,
	ss * 2 + ws * 4,
	ss * 2 + ws * 5,
	ss * 2 + ws * 6,
	ss * 2 + ws * 7,
}

Size6Key = ss * 2 + ws * 6
Size8Key = ss * 2 + ws * 8

if PlayerSide == 1 then
	GearStartX5K = GearStartX5KP1
	GearStartX7K = GearStartX7KP1
else
	GearStartX5K = GearStartX5KP2
	GearStartX7K = GearStartX7KP2
end

if ScratchSide == 1 then
	C6Relative = C6RelativeLS
	C8Relative = C8RelativeLS
else
	C6Relative = C6RelativeRS
	C8Relative = C8RelativeRS
end

-- Channel 6 absolute positions for player 1
C6P1 = {}
for k,v in ipairs(C6Relative) do
		C6P1[#C6P1 + 1] = GearStartX5KP1 + v
end

-- Channel 6 absolute positions for player 2
C6P2 = {}
for k,v in ipairs(C6Relative) do
	C6P2[#C6P2 + 1] = GearStartX5KP2 + v
end

-- Channel 8 absolute positions for player 1
C8P1 = {}
for k,v in ipairs(C8Relative) do
	C8P1[#C8P1 + 1] = GearStartX7KP1 + v
end

-- Channel 8 absolute positions for player 2
C8P2 = {}
for k,v in ipairs(C8Relative) do
		C8P2[#C8P2 + 1] = GearStartX7KP2 + v
end

if PlayerSide == 1 then
	C6P = C6P1
	C8P = C8P1
elseif PlayerSide == 2 then
	C6P = C6P2
	C8P = C8P2
end

if Channels == 6 then
	GearStartXP1 = GearStartX5KP1
	GearStartXP2 = GearStartX7KP2
	GearWidth = Size6Key
	CP1 = C6P1
	CP2 = C6P2
	GearStartX = GearStartX5K
else
	GearStartXP1 = GearStartX7KP1
	GearStartXP2 = GearStartX7KP2
	GearWidth = Size8Key
	CP1 = C8P1
	CP2 = C8P2
	GearStartX = GearStartX7K
end

Channels6Common = {
	-- Note Images
	Key1Image = "s",
	Key2Image = "1",
	Key3Image = "2",
	Key4Image = "3",
	Key5Image = "4",
	Key6Image = "5",
	NoteHeight = NoteHeight,
	
	-- Lane Widths
	Key1Width = Channels6Sizes[1],
	Key2Width = Channels6Sizes[2],
	Key3Width = Channels6Sizes[3],
	Key4Width = Channels6Sizes[4],
	Key5Width = Channels6Sizes[5],
	Key6Width = Channels6Sizes[6],

	GearWidth = Size6Key,
	BarlineWidth = Size6Key,
}

-- Auto positions by player side
Channels6P1 = {
	GearStartX = GearStartX5KP1,
	FieldStartX = C6P1[2] / SkinScale - ws / SkinScale, -- Unscaled for csv files
	ScratchX = C6P1[1] / SkinScale - ss / SkinScale,
	LaneSepX = GearStartX5KP1 / SkinScale - 40,
	LaneSep2X = GearStartX5KP1 / SkinScale + Size6Key / SkinScale,

	-- Lane positions
	Key1X = C6P1[1],
	Key2X = C6P1[2],
	Key3X = C6P1[3],
	Key4X = C6P1[4],
	Key5X = C6P1[5],
	Key6X = C6P1[6],
}

Channels6P2 = {
	GearStartX = GearStartX5KP2,
	FieldStartX = C6P2[2] / SkinScale - ws / SkinScale, -- Unscaled for csv files
	ScratchX = C6P2[1] / SkinScale - ss / SkinScale,
	LaneSepX = GearStartX5KP2 / SkinScale - 40,
	LaneSep2X = GearStartX5KP2 / SkinScale + Size6Key / SkinScale,

	-- Lane positions
	Key1X = C6P2[1],
	Key2X = C6P2[2],
	Key3X = C6P2[3],
	Key4X = C6P2[4],
	Key5X = C6P2[5],
	Key6X = C6P2[6],
}

Channels8Common = {
	  GearWidth = Size8Key,
	BarlineWidth = Size8Key,
	NoteHeight = NoteHeight,
		-- Lane Widths
	Key1Width = Channels8Sizes[1],
	Key2Width = Channels8Sizes[2],
	Key3Width = Channels8Sizes[3],
	Key4Width = Channels8Sizes[4],
	Key5Width = Channels8Sizes[5],
	Key6Width = Channels8Sizes[6],
	Key7Width = Channels8Sizes[7],
	  Key8Width = Channels8Sizes[8],

		-- Note Images
	Key1Image = "s",
	Key2Image = "1",
	Key3Image = "2",
	Key4Image = "3",
	Key5Image = "4",
	Key6Image = "5",
	Key7Image = "6",
	  Key8Image = "7",
	}

	Channels8P1 = {
		GearStartX = GearStartX7KP1,
	FieldStartX = C8P1[2] / SkinScale - ws / SkinScale, -- Unscaled for csv files
	ScratchX = C8P1[1] / SkinScale - ss / SkinScale,
	LaneSepX = GearStartX7KP1 / SkinScale - 40,
	LaneSep2X = GearStartX7KP1 / SkinScale + Size8Key / SkinScale,

	-- Lane positions
	Key1X = C8P1[1],
	Key2X = C8P1[2],
	Key3X = C8P1[3],
	Key4X = C8P1[4],
	Key5X = C8P1[5],
	Key6X = C8P1[6],
	Key7X = C8P1[7],
	  Key8X = C8P1[8],
}

Channels8P2 = {
	GearStartX = GearStartX7KP2,
	FieldStartX = C8P2[2] / SkinScale - ws / SkinScale, -- Unscaled for csv files
	ScratchX = C8P2[1] / SkinScale - ss / SkinScale,
	LaneSepX = GearStartX7KP2 / SkinScale - 40,
	LaneSep2X = GearStartX7KP2 / SkinScale + Size8Key / SkinScale,

	-- Lane positions
	Key1X = C8P2[1],
	Key2X = C8P2[2],
	Key3X = C8P2[3],
	Key4X = C8P2[4],
	Key5X = C8P2[5],
	Key6X = C8P2[6],
	Key7X = C8P2[7],
	Key8X = C8P2[8],
}

print ("Noteskin table: ", Noteskin)
Noteskin = Noteskin or {}

if PlayerSide == 1 then
  Noteskin[6] = table.join(Channels6P1, Channels6Common)
	Noteskin[8] = table.join(Channels8P1, Channels8Common)
else
	Noteskin[6] = table.join(Channels6P2, Channels6Common)
	Noteskin[8] = table.join(Channels8P2, Channels8Common)
end

print ("returning defs table...")
return Noteskin
