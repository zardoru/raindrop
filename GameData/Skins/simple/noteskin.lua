if Notes.Channels ~= 8 then
	fallback_require("noteskin")
	return
end

Lanes = Notes.Channels

skin_require("custom_defs")
-- All notes have their origin centered.

XR = 1360 / 1280
YR = 768 / 720
normalNotes = {}
holdBodies = {}

function setNoteStuff(note, i)
	note.Width = Noteskin[Lanes]['Key' .. i .. 'Width'] * XR
	note.X = Noteskin[Lanes]['Key' .. i .. 'X'] * XR
	note.Height = NoteHeight * YR
	note.Layer = 14
	note.Lighten = 1
	note.LightenFactor = 0
end

function Init()
	for i=1,Lanes do
		normalNotes[i] = Object2D()
		local note = normalNotes[i]
		local image = 'assets/n' .. Noteskin[8]["Key" .. i .. "Image"] .. ".png"
		
		print ("Set image " .. image)
		note.Texture = image
		setNoteStuff(note, i)
		
		holdBodies[i] = Object2D()
		note = holdBodies[i]
		
		image = 'assets/l'.. Noteskin[8]["Key" .. i .. "Image" ] .. ".png"
		print ("Set image " .. image)
		note.Texture = image
		setNoteStuff(note, i)
	end
end

function Update(delta, beat)
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
	
	if active_level == 2 then
		note.LightenFactor = 1
		note.Red = 1
		note.Green = 1
		note.Blue = 1
	elseif active_level == 1 then
		note.LightenFactor = 0
		note.Red = 1
		note.Green = 1
		note.Blue = 1
	elseif active_level == 0 then
		note.LightenFactor = 0
		note.Red = 0.5
		note.Green = 0.5
		note.Blue = 0.5
	end
	
	Notes:Render(note)
end

function drawMineInternal(lane, loc, frac)
	-- stub while mines are accounted in the scoring system.
end

-- From now on, only engine variables are being set.
-- Barline
Notes.BarlineEnabled = 1
Notes.BarlineOffset = NoteHeight / 2
Notes.BarlineStartX = GearStartX
Notes.BarlineWidth = Noteskin[Lanes].BarlineWidth * XR
Notes.JudgmentY = ScreenHeight - 485 * YR
Notes.DecreaseHoldSizeWhenBeingHit = 1
Notes.DanglingHeads = 1

-- How many extra units do you require so that the whole bounding box is accounted
-- when determining whether to show this note or not.
Notes.NoteScreenSize = NoteHeight / 2

DrawNormal = drawNormalInternal
DrawFake = drawNormalInternal
DrawLift = drawNormalInternal 
DrawMine = drawMineInternal

DrawHoldHead = drawNormalInternal
DrawHoldTail = drawNormalInternal
DrawHoldBody = drawHoldBodyInternal
