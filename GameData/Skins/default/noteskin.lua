skin_require("noteskin_defs.lua")

normalNotes = {}

for i=1,Lanes do
	normalNotes[i] = Object2D()
	local note = normalNotes[i]
	note.Image = Noteskin[Lanes]['Key' .. i .. 'Image']
	note.Width = Noteskin[Lanes]['Key' .. i .. 'Width']
	note.X = Noteskin[Lanes]['Key' .. i .. 'X']
	note.Height = NoteHeight
	note.Layer = 14
end

function drawNormalInternal(lane, loc)
		local note = normalNotes[lane + 1]
		note.Y = loc
		Render(note)
end

function drawHoldBodyInternal(lane, loc, size)

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