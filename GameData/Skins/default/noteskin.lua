skin_require("noteskin_defs.lua")

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
	
	Render(note)
end

function drawMineInternal(lane, loc, frac)
	
end

-- From now on, only engine variables are being set.
-- Barline
BarlineEnabled = 1
BarlineOffset = NoteHeight / 2
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