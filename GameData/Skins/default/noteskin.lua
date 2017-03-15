game_require "noteskin_defs"

-- All notes have their origin centered.

normalNotes = {}
holdBodies = {}

local TT = Notes.Channels == 6 or Notes.Channels == 8

if Player.Turntable and TT then
  skin = NoteskinSpecial
else
  skin = Noteskin
end
function setNoteStuff(note, i)
	note.Width = skin[Notes.Channels]['Key' .. i .. 'Width']
	note.X = skin[Notes.Channels]['Key' .. i .. 'X']
	note.Height = skin[Notes.Channels].NoteHeight
	note.Layer = 14
	note.Lighten = true
	note.LightenFactor = 0
end

function Init()

	for i=1,Notes.Channels do
		normalNotes[i] = Object2D()
		local note = normalNotes[i]
		note.Texture = skin[Notes.Channels]['Key' .. i .. 'Image']
		setNoteStuff(note, i)
		
		holdBodies[i] = Object2D()
		note = holdBodies[i]
		note.Texture = skin[Notes.Channels]['Key' .. i .. 'HoldImage']
		setNoteStuff(note, i)
	end
end

function Update(delta, beat)
	local fraction = 1 - (beat - math.floor(beat))
	for i=1, Notes.Channels do 
		local note = normalNotes[i]
		note.LightenFactor = fraction
	end 
end 

function drawNormalInternal(lane, loc, frac, active_level)
	local note = normalNotes[lane + 1]
	note.Y = loc
	
	if active_level ~= 3 then
		Notes:Render(note)
	end
end

-- 1 is enabled. 2 is being pressed. 0 is failed. 3 is succesful hit.
function drawHoldBodyInternal(lane, loc, size, active_level)
	local note = holdBodies[lane + 1]
	note.Y = loc
	note.Height = size
	
	if active_level == 0 then
		note.Red = 0.5
		note.Blue = 0.5
		note.Green = 0.5
	else
		note.Red = 1
		note.Blue = 1
		note.Green = 1
	end
	
	if active_level ~= 3 then
		Notes:Render(note)
	end
end

function drawMineInternal(lane, loc, frac)
	-- stub while mines are accounted in the scoring system.
end

-- From now on, only engine variables are being set.
-- Barline
--game_require "debug"
Notes.BarlineEnabled = 1
Notes.BarlineOffset = skin[Notes.Channels].NoteHeight / 2
Notes.BarlineStartX = GearStartX

Notes.BarlineWidth = skin[Notes.Channels].BarlineWidth

local jy = skin[Notes.Channels].GearHeight + skin[Notes.Channels].NoteHeight / 2
Notes.JudgmentY = jy
Notes.DecreaseHoldSizeWhenBeingHit = true
Notes.DanglingHeads = true

-- How many extra units do you require so that the whole bounding box is accounted
-- when determining whether to show this note or not.
Notes.NoteScreenSize = skin[Notes.Channels].NoteHeight / 2

DrawNormal = drawNormalInternal
DrawFake = drawNormalInternal
DrawLift = drawNormalInternal 
DrawMine = drawMineInternal

DrawHoldHead = drawNormalInternal
DrawHoldTail = drawNormalInternal
DrawHoldBody = drawHoldBodyInternal
