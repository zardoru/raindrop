Lanes = Player.Channels

if Lanes ~= 8 and Lanes ~= 6 then
	fallback_require("noteskin")
	return
end

require "TextureAtlas"
Noteskin, a, b = skin_require "custom_defs"

print(Noteskin, a, b)
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
	ret.Texture = "assets/" .. NoteAtlas.File
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

	ptop.Texture = "assets/" .. atlas.File
	pmid.Texture = "assets/" .. atlas.File
	pbot.Texture = "assets/" .. atlas.File

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
		Notes:Render(notes[lane])
end

function drawHoldTopInternal(lane, loc, frac, active_level)
	local note
	lane = lane + 1
	if active_level == 2 then
		note = longnotes[lane].on.top
	else
		note = longnotes[lane].off.top
	end

	if active_level == 3 or active_level == 0 then 
		return
	end

	note.Y = loc
	Notes:Render(note)
end

function drawHoldBotInternal(lane, loc, frac, active_level)
	local note
	lane = lane + 1
	if active_level == 2 then
		note = longnotes[lane].on.bot
	else
		note = longnotes[lane].off.bot
	end

	if active_level == 3 or active_level == 0 then 
		return
	end

	note.Y = loc
	Notes:Render(note)
end

function drawHoldBodyInternal(lane, loc, size, active_level)
	local note
	lane = lane + 1
	if active_level == 2 then
		note = longnotes[lane].on.mid
	else
		note = longnotes[lane].off.mid
	end

	if active_level == 3 or active_level == 0 then 
		return
	end

	note.Y = loc
	note.Height = size
	Notes:Render(note)
end

function drawMineInternal(lane, loc, frac)
	-- stub while mines are accounted in the scoring system.
end

-- From now on, only engine variables are being set.
-- Barline
Notes.BarlineEnabled = 1
Notes.BarlineOffset = Noteskin[Lanes].NoteHeight / 2

print ("Noteskin lanes is ")
Notes.BarlineStartX = Noteskin[Lanes].GearStartX
Notes.BarlineWidth = Noteskin[Lanes].BarlineWidth
Notes.JudgmentY = ScreenHeight - (1560 * SkinScale - NoteHeight / 2)
Notes.DecreaseHoldSizeWhenBeingHit = 1
Notes.DanglingHeads = 1

-- How many extra units do you require so that the whole bounding box is accounted
-- when determining whether to show this note or not.
Notes.NoteScreenSize = Noteskin[Lanes].NoteHeight / 2

DrawNormal = drawNormalInternal
DrawFake = drawNormalInternal
DrawLift = drawNormalInternal
DrawMine = drawMineInternal

DrawHoldHead = drawHoldTopInternal
DrawHoldTail = drawHoldBotInternal
DrawHoldBody = drawHoldBodyInternal
