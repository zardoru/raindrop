game_require("noteskin_defs")
game_require("TextureAtlas")
-- All notes have their origin centered.

normalNotes = {}
holdBodies = {}

Atlas = TextureAtlas:new("GameData/Skins/ftb2d/assets/notes.csv")
AtlasHolds = TextureAtlas:new("Gamedata/Skins/ftb2d/assets/holds.csv")
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
	local image = Noteskin[Lanes]['Key' .. i .. 'Image'] .. ".png"
	note.Image = Atlas.File
	Atlas:SetObjectCrop(note, image)
	setNoteStuff(note, i)
	
	holdBodies[i] = Object2D()
	note = holdBodies[i]
	
	image = Noteskin[Lanes]['Key' .. i .. 'Image'] .. ".png"
	note.Image = AtlasHolds.File
	AtlasHolds:SetObjectCrop(note, image)
	setNoteStuff(note, i)
end

function Update(delta, beat)
end 

function drawNormalInternal(lane, loc, frac, active_level)
	local note = normalNotes[lane + 1]
	note.Y = loc
	
	if active_level ~= 3 then
		Render(note)
	end
end

-- 1 is enabled. 2 is being pressed. 0 is failed. 3 is succesful hit.
function drawHoldBodyInternal(lane, loc, size, active_level)
	local note = holdBodies[lane + 1]
	note.Y = loc
	note.Height = size
	
	if active_level == 2 then
		note.LightenFactor = 1
	else
		note.LightenFactor = 0
	end
	
	if active_level ~= 3 and active_level ~= 0 then
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
JudgmentLineY = 228 + NoteHeight / 2
DecreaseHoldSizeWhenBeingHit = 1
DanglingHeads = 1

-- How many extra units do you require so that the whole bounding box is accounted
-- when determining whether to show this note or not.
NoteScreenSize = NoteHeight / 2

DrawNormal = drawNormalInternal
DrawFake = drawNormalInternal
DrawLift = drawNormalInternal 
DrawMine = drawMineInternal

DrawHoldHead = drawNormalInternal
DrawHoldTail = function (a,b,c,d) end
DrawHoldBody = drawHoldBodyInternal