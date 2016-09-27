game_require "librd"

Channels4Sizes = {
	100,
	100,
	100,
	100
}

Channels4Positions = {
	0,
	100,
	200,
	300
}

local gsx = 140
Channels4Positions = map(function(x) return x + gsx + 50 end, Channels4Positions)

GearWidthByChannels = {}

-- Only 4 channels supported in wafles4.
Channels4 = {
	GearStartX = gsx,
	NoteHeight = 100,
    GearHeight = 100,
    Key1Width = Channels4Sizes[1],
    Key2Width = Channels4Sizes[2],
    Key3Width = Channels4Sizes[3],
    Key4Width = Channels4Sizes[4],
    Key1X = Channels4Positions[1],
    Key2X = Channels4Positions[2],
    Key3X = Channels4Positions[3],
    Key4X = Channels4Positions[4],
    GearWidth = 400,
    BarlineWidth = 400,
    rotTable = {90, 0, 180, 270}
}

print "Setting custom"
Noteskin = Noteskin or {}
Noteskin[4] = Channels4
