if Channels ~= 8 and Channels ~= 6 then
	if Lanes ~= 8 and Lanes ~= 6 then
		return
	end
end

require "utils"
skin_require "skin_defs"

NoteHeight = 10
GearStartX5KP1 = 120
GearStartX7KP1 = 85
GearStartX5KP2 = 3000
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

-- start x, width list
-- assume first is scratch, 2nd and on from scratch
function generate_key_pos(sx, wl)
    local ret = {
        sx + 50 * SkinScale
    }
    
    -- current entry is previous entry + half-width of previous entry
    print (#wl)
    for i=2, #wl do
        table.insert(ret, ret[#ret] + wl[i - 1] / 2)
    end

    ret[#wl] = ret[#wl - 1] + wl[#wl] / 2
    return ret
end

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
    C6P2[#C6P1 + 1] = GearStartX5KP2 + v
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

-- Auto positions by player side
Channels6 = {
    GearStartX = GearStartX5K,
    FieldStartX = C6P[2] / SkinScale - ws / SkinScale, -- Unscaled for csv files
    ScratchX = C6P[1] / SkinScale - ss / SkinScale,
	GearWidth = Size6Key,
    BarlineWidth = Size6Key,
    LaneSepX = GearStartX5K / SkinScale - 40,
    LaneSep2X = GearStartX5K / SkinScale + Size6Key / SkinScale,

    -- Note Images
    Key1Image = "s",
    Key2Image = "1",
    Key3Image = "2",
    Key4Image = "3",
    Key5Image = "4",
    Key6Image = "5",

    -- Lane positions
    Key1X = C6P[1],
    Key2X = C6P[2],
    Key3X = C6P[3],
    Key4X = C6P[4],
    Key5X = C6P[5],
    Key6X = C6P[6],
    
    -- Lane Widths
    Key1Width = Channels6Sizes[1],
    Key2Width = Channels6Sizes[2],
    Key3Width = Channels6Sizes[3],
    Key4Width = Channels6Sizes[4],
    Key5Width = Channels6Sizes[5],
    Key6Width = Channels6Sizes[6],
}

Channels8 = {
    GearStartX = GearStartX7K,
    FieldStartX = C8P[2] / SkinScale - ws / SkinScale, -- Unscaled for csv files
    ScratchX = C8P[1] / SkinScale - ss / SkinScale,
	GearWidth = Size8Key,
    BarlineWidth = Size8Key,
    LaneSepX = GearStartX7K / SkinScale - 40,
    LaneSep2X = GearStartX7K / SkinScale + Size8Key / SkinScale,

    -- Note Images
    Key1Image = "s",
    Key2Image = "1",
    Key3Image = "2",
    Key4Image = "3",
    Key5Image = "4",
    Key6Image = "5",
    Key7Image = "6",
	Key8Image = "7",

    -- Lane positions
    Key1X = C8P[1],
    Key2X = C8P[2],
    Key3X = C8P[3],
    Key4X = C8P[4],
    Key5X = C8P[5],
    Key6X = C8P[6],
    Key7X = C8P[7],
	Key8X = C8P[8],

    -- Lane Widths
    Key1Width = Channels8Sizes[1],
    Key2Width = Channels8Sizes[2],
    Key3Width = Channels8Sizes[3],
    Key4Width = Channels8Sizes[4],
    Key5Width = Channels8Sizes[5],
    Key6Width = Channels8Sizes[6],
    Key7Width = Channels8Sizes[7],
	Key8Width = Channels8Sizes[8]
}

Noteskin = Noteskin or {}
Noteskin[6] = Channels6
Noteskin[8] = Channels8

return Noteskin