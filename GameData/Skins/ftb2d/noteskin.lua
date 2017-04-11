if Notes.Channels > 7 then
	fallback_require("noteskin")
	return
end

skin_require("custom_defs")
game_require("TextureAtlas")
-- All notes have their origin centered.

normalNotes = {}
holdBodies = {}

function setNoteStuff(note, i)
	note.Width = Noteskin[Notes.Channels]['Key' .. i .. 'Width']
	note.X = Noteskin[Notes.Channels]['Key' .. i .. 'X']
	note.Height = NoteHeight
	note.Layer = 14
	note.Lighten = 1
	note.LightenFactor = 0
end

function Init()
	Atlas = TextureAtlas:new("GameData/Skins/ftb2d/assets/notes.csv")
	AtlasHolds = TextureAtlas:new("Gamedata/Skins/ftb2d/assets/holds.csv")
	for i=1,Notes.Channels do
	  local MapLane = Noteskin[Notes.Channels].Map[i]
		normalNotes[i] = Object2D()
		local note = normalNotes[i]
		local image = Noteskin[7]['Key' .. MapLane .. 'Image'] .. ".png"
		note.Texture = Atlas.File
		Atlas:SetObjectCrop(note, image)
		setNoteStuff(note, i)
		
		holdBodies[i] = Object2D()
		note = holdBodies[i]
		
		image = Noteskin[7]['Key' .. MapLane .. 'Image'] .. ".png"
		note.Texture = AtlasHolds.File
		AtlasHolds:SetObjectCrop(note, image)
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
	else
		note.LightenFactor = 0
	end
	
	if active_level ~= 3 and active_level ~= 0 then
		Notes:Render(note)
	end
end

function drawMineInternal(lane, loc, frac)
	-- stub while mines are accounted in the scoring system.
end

-- From now on, only engine variables are being set.
-- Barline
Notes.BarlineEnabled = 1
Notes.BarlineOffset = NoteHeight / 2
Notes.BarlineStartX = GearStartX
Notes.BarlineWidth = Noteskin[7].BarlineWidth
Notes.JudgmentY = 228 + NoteHeight / 2
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
DrawHoldTail = nil
DrawHoldBody = drawHoldBodyInternal
