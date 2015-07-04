skin_require("noteskin_defs.lua")

-- All notes have their origin centered.

normalNotes = {}
holdBodies = {}

function setNoteStuff(note, i)
	note.Width = Noteskin[Lanes]['Key' .. i .. 'Width']
	note.X = Noteskin[Lanes]['Key' .. i .. 'X']
	note.Height = NoteHeight
	note.Layer = 14
	note.Lighten = 1
	note.LightenFactor = 0
end

for i=1,Lanes do
	normalNotes[i] = Object2D()
	local note = normalNotes[i]
	note.Image = Noteskin[Lanes]['Key' .. i .. 'Image']
	setNoteStuff(note, i)
	
	holdBodies[i] = Object2D()
	note = holdBodies[i]
	note.Image = Noteskin[Lanes]['Key' .. i .. 'HoldImage']
	setNoteStuff(note, i)
end

function Update(delta, beat)
	local fraction = 1 - (beat - math.floor(beat))
	for i=1, Lanes do 
		local note = normalNotes[i]
		note.LightenFactor = fraction
	end 
end 

function drawNormalInternal(lane, loc, frac)
	local note = normalNotes[lane + 1]
	note.Y = loc
	Render(note)
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
	
	if active_level != 3 then
		Render(note)
	end
end

function drawMineInternal(lane, loc, frac)
	-- stub while mines are accounted in the scoring system.
end

-- From now on, only engine variables are being set.
-- Barline
BarlineEnabled = 1
BarlineOffset = NoteHeight / 2
BarlineStartX = GearStartX
BarlineWidth = Noteskin[Lanes].BarlineWidth
JudgmentLineY = Noteskin[Lanes].GearHeight

-- How many extra units do you require so that the whole bounding box is accounted
-- when determining whether to show this note or not.
NoteScreenSize = NoteHeight / 2

DrawNormal = drawNormalInternal
DrawFake = drawNormalInternal
DrawLift = drawNormalInternal 
DrawMine = drawMineInternal

DrawHoldHead = drawNormalInternal
DrawHoldTail = drawNormalInternal
DrawHoldBody = drawHoldBodyInternal