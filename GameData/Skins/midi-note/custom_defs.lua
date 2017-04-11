require "utils"

print "Running midi-note custom defs"
Channels4Sizes = {
	100,
	100,
	100,
	100
}

Channels4Positions = {
	65,
	165,
	265,
	365
}

-- Only 4 channels supported in wafles4.
Channels4 = {
	NoteHeight = 100,
	GearHeight = 100,
	GearStartX = 15,
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
	NoteHeight = 100
}

Noteskin = Noteskin or {}
Noteskin[4] = Channels4