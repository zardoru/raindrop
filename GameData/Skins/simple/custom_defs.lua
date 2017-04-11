if not Game or Game:GetPlayer(0).Channels ~= 8 then
	if Notes.Channels ~= 8 then
		return
	end
end

require "utils"

GearStartX = 48

NoteHeight = 10

Channels8Sizes = {
	78,
	44,
	34,
	44,
	34,
	44,
	34,
	44
}

Channels8Positions = {}

Sizeup(Channels8Positions, Channels8Sizes, 8, 3) -- 3px padding

Channels8 = {
	GearStartX = GearStartX,
	GearWidth = GearWidthByChannels[8],
    BarlineWidth = GearWidthByChannels[8],
	BeamY = 139,

    -- Note Images
    Key1Image = "tt",
    Key2Image = "k1",
    Key3Image = "k2",
    Key4Image = "k1",
    Key5Image = "k2",
    Key6Image = "k1",
    Key7Image = "k2",
	Key8Image = "k1",

    -- Lane positions
    Key1X = Channels8Positions[1],
    Key2X = Channels8Positions[2],
    Key3X = Channels8Positions[3],
    Key4X = Channels8Positions[4],
    Key5X = Channels8Positions[5],
    Key6X = Channels8Positions[6],
    Key7X = Channels8Positions[7],
	Key8X = Channels8Positions[8],

	Key1XB = Channels8Positions[1] - Channels8Sizes[1] / 2,
    Key2XB = Channels8Positions[2] - Channels8Sizes[2] / 2,
    Key3XB = Channels8Positions[3] - Channels8Sizes[3] / 2,
    Key4XB = Channels8Positions[4] - Channels8Sizes[4] / 2,
    Key5XB = Channels8Positions[5] - Channels8Sizes[5] / 2,
    Key6XB = Channels8Positions[6] - Channels8Sizes[6] / 2,
    Key7XB = Channels8Positions[7] - Channels8Sizes[7] / 2,
	Key8XB = Channels8Positions[8] - Channels8Sizes[8] / 2,
	
    -- Lane Widths
    Key1Width = Channels8Sizes[1],
    Key2Width = Channels8Sizes[2],
    Key3Width = Channels8Sizes[3],
    Key4Width = Channels8Sizes[4],
    Key5Width = Channels8Sizes[5],
    Key6Width = Channels8Sizes[6],
    Key7Width = Channels8Sizes[7],
	Key8Width = Channels8Sizes[8],
    Map = {1, 2, 3, 4, 5, 6, 7, 8}
}

Noteskin = Noteskin or {}
Noteskin[8] = Channels8