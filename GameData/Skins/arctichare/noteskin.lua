if Lanes ~= 8 and Lanes ~= 6 then
	fallback_require("noteskin")
	return
end

require "TextureAtlas"
skin_require "custom_defs"
-- All notes have their origin centered.

notes = {}
longnotes = {}

function SetCommonNoteStuff(note, i)
	note.Width = Noteskin[Lanes]['Key' .. i .. 'Width']
	note.X = Noteskin[Lanes]['Key' .. i .. 'X']
	note.Height = NoteHeight
	note.Layer = 14
end

function MakeNote(i)
	ret = Object2D()
	ret.Image = "assets/" .. NoteAtlas.File
	print (Noteskin)
	print (Noteskin[Lanes])
	print ('Key' .. i .. 'Image')
	NoteAtlas:SetObjectCrop(ret, "note-" .. Noteskin[Lanes]['Key' .. i .. 'Image'] .. ".png")
	SetCommonNoteStuff(ret, i)
	return ret
end

function MakeLongNote(i, n, isOn)
	ptop = Object2D()
	pmid = Object2D()
	pbot = Object2D()

	local atlas
	local txt
	if isOn then
		txt = "on"
		atlas = LNAtlas.on
	elseif not isOn then
		txt = "off"
		atlas = LNAtlas.off
	end	 

	ptop.Image = "assets/" .. atlas.File
	pmid.Image = "assets/" .. atlas.File
	pbot.Image = "assets/" .. atlas.File

	local fntop = "note-long-top-" .. txt .. "-" .. i .. ".png"
	atlas:SetObjectCrop(ptop, fntop)

	local fnmid = "note-long-mid-" .. txt .. "-" .. i .. ".png"
	atlas:SetObjectCrop(pmid, fnmid)

	local fnbot = "note-long-bot-" .. txt .. "-" .. i .. ".png"
	atlas:SetObjectCrop(pbot, fnbot)

	SetCommonNoteStuff(ptop, n)
	SetCommonNoteStuff(pmid, n)
	SetCommonNoteStuff(pbot, n)
	return {
		top = ptop,
		mid = pmid,
		bot = pbot
	}
end

function Init()
	NoteAtlas = TextureAtlas:skin_new("assets/note-normal.csv")
	
	LNAtlas = {
		on = TextureAtlas:skin_new("assets/note-long-on.csv"),
		off = TextureAtlas:skin_new("assets/note-long-off.csv")
	}

	BombAtlas = TextureAtlas:skin_new("assets/note-bomb.csv")

	for i=1,Lanes do
		local keypic = Noteskin[Lanes]["Key" .. i .. "Image"]
		-- normal hit notes
		notes[i] = MakeNote(i)
		
		-- long notes (off by default)
		longnotes[i] = {}
		longnotes[i].on = MakeLongNote(keypic, i, true)
		longnotes[i].off = MakeLongNote(keypic, i, false)
	end
end

function Update(delta, beat)
end 

-- 1 is enabled. 2 is being pressed. 0 is failed. 3 is succesful hit.
function drawNormalInternal(lane, loc, frac, active_level)
		lane = lane + 1
		notes[lane].Y = loc
		Render(notes[lane])
end

function drawHoldTopInternal(lane, loc, frac, active_level)
	local note
	lane = lane + 1
	if active_level == 2 then
		note = longnotes[lane].on.top
	else 
		note = longnotes[lane].off.top
	end

	note.Y = loc
	Render(note)
end

function drawHoldBotInternal(lane, loc, frac, active_level)
	local note
	lane = lane + 1
	if active_level == 2 then
		note = longnotes[lane].on.bot
	else 
		note = longnotes[lane].off.bot
	end

	note.Y = loc
	Render(note)
end

function drawHoldBodyInternal(lane, loc, size, active_level)
	local note
	lane = lane + 1
	if active_level == 2 then
		note = longnotes[lane].on.mid
	else 
		note = longnotes[lane].off.mid
	end

	note.Y = loc
	note.Height = size
	Render(note)
end

function drawMineInternal(lane, loc, frac)
	-- stub while mines are accounted in the scoring system.
end

-- From now on, only engine variables are being set.
-- Barline
BarlineEnabled = 1
BarlineOffset = NoteHeight / 2
BarlineStartX = Noteskin[Lanes].GearStartX
BarlineWidth = Noteskin[Lanes].BarlineWidth
JudgmentLineY = ScreenHeight - (1560 * SkinScale - NoteHeight / 2)
DecreaseHoldSizeWhenBeingHit = 1
DanglingHeads = 1

-- How many extra units do you require so that the whole bounding box is accounted
-- when determining whether to show this note or not.
NoteScreenSize = NoteHeight / 2

DrawNormal = drawNormalInternal
DrawFake = drawNormalInternal
DrawLift = drawNormalInternal 
DrawMine = drawMineInternal

DrawHoldHead = drawHoldTopInternal
DrawHoldTail = drawHoldBotInternal
DrawHoldBody = drawHoldBodyInternal
