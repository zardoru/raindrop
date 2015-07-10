require "utils"

GearStartX = 40

NoteHeight = 10

Channels7Sizes = {
	40,
	40,
	40,
	40,
	40,
	40,
	40
}

Channels7Positions = {}

Sizeup(Channels7Positions, Channels7Sizes, 7)

Channels7 = {
	GearWidth = GearWidthByChannels[7],
    BarlineWidth = GearWidthByChannels[7],

    -- Note Images
    Key1Image = 1,
    Key2Image = 2,
    Key3Image = 3,
    Key4Image = 4,
    Key5Image = 3,
    Key6Image = 2,
    Key7Image = 1,

    -- Lane positions
    Key1X = Channels7Positions[1],
    Key2X = Channels7Positions[2],
    Key3X = Channels7Positions[3],
    Key4X = Channels7Positions[4],
    Key5X = Channels7Positions[5],
    Key6X = Channels7Positions[6],
    Key7X = Channels7Positions[7],

    -- Lane Widths
    Key1Width = Channels7Sizes[1],
    Key2Width = Channels7Sizes[2],
    Key3Width = Channels7Sizes[3],
    Key4Width = Channels7Sizes[4],
    Key5Width = Channels7Sizes[5],
    Key6Width = Channels7Sizes[6],
    Key7Width = Channels7Sizes[7],
    Map = {1, 2, 3, 4, 5, 6, 7}
}

Channels4 = {
  Key1X = Channels7Positions[2],
  Key2X = Channels7Positions[3],
  Key3X = Channels7Positions[5],
  Key4X = Channels7Positions[6],
  
  Key1Width = Channels7Sizes[1],
  Key2Width = Channels7Sizes[2],
  Key3Width = Channels7Sizes[3],
  Key4Width = Channels7Sizes[4],
  
  Map = {2, 3, 5, 6}
}

Channels6 = {
    Key1X = Channels7Positions[1],
    Key2X = Channels7Positions[2],
    Key3X = Channels7Positions[3],
    Key4X = Channels7Positions[5],
    Key5X = Channels7Positions[6],
    Key6X = Channels7Positions[7],
    
    Key1Width = Channels7Sizes[1],
    Key2Width = Channels7Sizes[2],
    Key3Width = Channels7Sizes[3],
    Key4Width = Channels7Sizes[4],
    Key5Width = Channels7Sizes[5],
    Key6Width = Channels7Sizes[6],
    
    Map = {1, 2, 3, 5, 6, 7}
}

Channels5 = {
  Key1X = Channels7Positions[2],
  Key2X = Channels7Positions[3],
  Key3X = Channels7Positions[4],
  Key4X = Channels7Positions[5],
  Key5X = Channels7Positions[6],
  
  Key1Width = Channels7Sizes[1],
  Key2Width = Channels7Sizes[2],
  Key3Width = Channels7Sizes[3],
  Key4Width = Channels7Sizes[4],
  Key5Width = Channels7Sizes[5],
  
  Map = {2, 3, 4, 5, 6}
}


Noteskin = Noteskin or {}
Noteskin[4] = Channels4
Noteskin[5] = Channels5
Noteskin[6] = Channels6
Noteskin[7] = Channels7