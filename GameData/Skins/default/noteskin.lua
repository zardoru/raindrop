skin_require("noteskin_defs.lua")

normalNotes = {}
holdBodies = {}

offset = NoteHeight / 2
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

function drawNormalInternal(lane, loc)
		local note = normalNotes[lane + 1]
		note.Y = loc - offset
		Render(note)
end

function drawHoldBodyInternal(lane, loc, size)
	local note = holdBodies[lane + 1]
	note.Y = loc - offset
	note.Height = size
	Render(note)
end

-- From now on, only engine variables are being set.
-- Barline
BarlineEnabled = 1
BarlineOffset = 0
BarlineStartX = GearStartX
BarlineWidth = Noteskin[Lanes].BarlineWidth
JudgmentLineY = Noteskin[Lanes].GearHeight

DrawNormal = drawNormalInternal
DrawFake = drawNormalInternal
DrawLift = drawNormalInternal 
DrawMine = drawMineInternal

DrawHoldHead = drawNormalInternal
DrawHoldTail = drawNormalInternal
DrawHoldBody = drawHoldBodyInternal