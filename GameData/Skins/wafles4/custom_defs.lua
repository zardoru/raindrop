GearStartX = 15

Channels4Sizes = {
	100,
	100,
	100,
	100
}
Channels4Positions = {}

GearWidthByChannels = {}
-- Calculate the positions of lanes based off their sizes and a start position.
function Sizeup(Pos, Size, Num)
	local GearWidthChannel = 0
	
	for i=1, Num do
		GearWidthChannel = GearWidthChannel + Size[i]
	end
	
	GearWidthByChannels[Num] = GearWidthChannel
	
	Pos[1] = Size[1] / 2 + GearStartX
	for i=2, Num do
		Pos[i] = Pos[i-1] + Size[i-1] / 2 + Size[i] / 2
	end
end

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

Noteskin = {}
Noteskin[4] = Channels4