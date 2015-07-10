require "utils"

if Channels == 4 or Lanes == 4 then
	cnt = 1
else
	cnt = nil
end

if cnt == nil then
	return
end

GearStartX = 15

Channels4Sizes = {
	100,
	100,
	100,
	100
}
Channels4Positions = {}

Sizeup(Channels4Positions, Channels4Sizes, 4)

-- Note height.
NoteHeight = 100

-- Only 4 channels supported in wafles4.
Channels4 = {
    GearHeight = 100,
    Key1Width = Channels4Sizes[1],
    Key2Width = Channels4Sizes[2],
    Key3Width = Channels4Sizes[3],
    Key4Width = Channels4Sizes[4],
    Key1X = Channels4Positions[1],
    Key2X = Channels4Positions[2],
    Key3X = Channels4Positions[3],
    Key4X = Channels4Positions[4],
	  GearWidth = GearWidthByChannels[4],
    BarlineWidth = GearWidthByChannels[4]
}

Noteskin = Noteskin or {}
Noteskin[4] = Channels4