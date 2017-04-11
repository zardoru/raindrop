
-- Wrap up the noteskin on a function in case we want to call whatever else.
function DoWafles()
	-- All notes have their origin centered.

	normalNotes = {}

	holdBodiesInactive = {}
	holdBodiesActive = {}

	holdTailsInactive = {}
	holdTailsActive = {}

	fTable = {1, 2, 3, 4, 6, 8, 16, 48}
	yTable = {}

	rotTable = {270, 180, 0, 90}
	tapSizePixels = 1024 / 8

	for i=1, 8 do
		yTable[fTable[i]] = { End = (i - 1) * tapSizePixels, Start = i * tapSizePixels }
	end

	function Init()

		function setNoteStuff(note, i)
			note.Width = Noteskin[4]['Key' .. i .. 'Width']
			note.X = Noteskin[4]['Key' .. i .. 'X']
			note.Height = Noteskin[4].NoteHeight
			note.Layer = 14
			note.Lighten = 1
			note.LightenFactor = 0
		end

		for i=1,Notes.Channels do
			normalNotes[i] = Object2D()
			local note = normalNotes[i]
			note.Texture = "_down tap note 1x8 (doubleres).png"
			setNoteStuff(note, i)
			
			holdBodiesInactive[i] = Object2D()
			note = holdBodiesInactive[i]
			note.Texture = "holdbodyinactive.png"
			setNoteStuff(note, i)
			
			holdBodiesActive[i] = Object2D()
			note = holdBodiesActive[i]
			note.Texture = "holdbodyactive.png"
			bodyHeight = note.Height
			setNoteStuff(note, i)
			
			holdTailsInactive[i] = Object2D()
			note = holdTailsInactive[i]
			note.Texture = "holdtailinactive.png"
			setNoteStuff(note, i)
			
			holdTailsActive[i] = Object2D()
			note = holdTailsActive[i]
			note.Texture = "holdtailactive.png"
			setNoteStuff(note, i)
		end
	end

	function drawHoldTailInternal(lane, loc, frac, active_level)
		local note;
		note = holdTailsInactive[lane + 1]
		
		if active_level ~= 0 then
			note = holdTailsActive[lane + 1]
		end
		
		if active_level == 2 then 
			note.LightenFactor = 1
		else
			note.LightenFactor = 0
		end
		
		
		if Player.Upscroll then
			note.Y = loc + Noteskin[4].NoteHeight / 2
			note.Rotation = 0
		else 
			note.Y = loc - Noteskin[4].NoteHeight / 2
			note.Rotation = 180
		end
		
		if active_level ~= 3 then
			Notes:Render(note)
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
		
		value = yTable[frac] or yTable[48]
		
		-- colorize note
		note:SetCropByPixels(0, 128, value.Start, value.End)
		note.Rotation = rotTable[lane + 1]
		if active_level ~= 3 then
			Notes:Render(note)
		end
	end

	-- 1 is enabled. 2 is being pressed. 0 is failed. 3 is succesful hit.
	function drawHoldBodyInternal(lane, loc, size, active_level)
		function do_draw(lane, loc, size, active_level)
			local note;
		
			if active_level == 0 then
				note = holdBodiesInactive[lane + 1]
				note.LightenFactor = 0
			else
				note = holdBodiesActive[lane + 1]
			end 
			
			if active_level == 2 then
				note.LightenFactor = 1
			else
				note.LightenFactor = 0
			end
		
			note.Y = loc
			note.Height = math.abs(size)
		
			-- force repeat texture
			note:SetCropByPixels(0, 128, 0, size)
			if active_level ~= 3 then
				Notes:Render(note)
			end
		end
		
		do_draw(lane, loc, size, active_level)
	end

	function drawHoldHeadInternal(lane, loc, frac, active_level)
		drawNormalInternal(lane, loc, frac, active_level)
	end

	function drawMineInternal(lane, loc, frac)
		-- stub while mines are accounted in the scoring system.
	end

	-- From now on, only engine variables are being set.
	-- Barline
	Notes.BarlineEnabled = false
	Notes.BarlineOffset = Noteskin[Notes.Channels].NoteHeight / 2
	Notes.BarlineStartX = Noteskin[Notes.Channels].GearStartX
	Notes.BarlineWidth = Noteskin[Notes.Channels].BarlineWidth
	Notes.JudgmentY = Noteskin[Notes.Channels].GearHeight
	Notes.DecreaseHoldSizeWhenBeingHit = true
	Notes.DanglingHeads = false

	-- How many extra units do you require so that the whole bounding box is accounted
	-- when determining whether to show this note or not.
	Notes.NoteScreenSize = Noteskin[Notes.Channels].NoteHeight / 2

	DrawNormal = drawNormalInternal
	DrawFake = drawNormalInternal
	DrawLift = drawNormalInternal 
	DrawMine = drawMineInternal

	DrawHoldHead = drawHoldHeadInternal
	DrawHoldTail = drawHoldTailInternal
	DrawHoldBody = drawHoldBodyInternal
end

if Notes.Channels == 4 then
	skin_require "custom_defs"
	DoWafles()
else
	fallback_require("noteskin")
end
