function doMidiNote()
	-- All notes have their origin centered.

	normalNotes = {}

	holdBodiesInactive = {}
	holdBodiesActive = {}

	holdTailsInactive = {}
	holdTailsActive = {}

	holdHeads = {}

	fTable = {1, 2, 3, 4, 6, 8, 16, 48}
	yTable = {}

	xTable = {}
	xTableFake = {}

	rotTable = {90, 0, 180, 270}
	rotTableNotes = {270, 180, 0, 90}
	tapSizePixels = 1024 / 8

	function setNoteStuff(note, i, rot)
		note.Width = Noteskin[Lanes]['Key' .. i .. 'Width']
		note.X = Noteskin[Lanes]['Key' .. i .. 'X']
		note.Height = NoteHeight
		note.Layer = 14
		note.Lighten = 1
		note.LightenFactor = 0
	  
	  if rot then
		note.Rotation = rot[i]
	  end
	end

	function Init()
		for i=1, 8 do
			yTable[fTable[i]] = { End = (i - 1) * tapSizePixels, Start = i * tapSizePixels }
		end

		for i=1, 4 do
		  local half = 512
		  local frame = (i - 1) * 128
		  local nextFrame = i * 128
		  xTable[i] = { Start = half + frame, End = half + nextFrame }
		  xTableFake[i] = { Start = frame, End = nextFrame }
		end
		for i=1,Lanes do
			normalNotes[i] = Object2D()
			local note = normalNotes[i]
			note.Image = "_Down Tap Note 8x8 (doubleres).png"
			setNoteStuff(note, i, rotTableNotes)
			
			holdBodiesInactive[i] = Object2D()
			note = holdBodiesInactive[i]
			note.Image = "Down Hold Body Active (doubleres).png"
			setNoteStuff(note, i)
			
			holdBodiesActive[i] = Object2D()
			note = holdBodiesActive[i]
			note.Image = "Down Hold Body Inactive (doubleres).png"
			bodyHeight = note.Height
			setNoteStuff(note, i)
			
			holdTailsInactive[i] = Object2D()
			note = holdTailsInactive[i]
			note.Image = "Down Hold BottomCap Inactive (doubleres).png"
			setNoteStuff(note, i)
			
			holdTailsActive[i] = Object2D()
			note = holdTailsActive[i]
		  note.Image = "Down Hold BottomCap Active (doubleres).png"
		  setNoteStuff(note, i, tailsRot)
			
			
		  
		  holdHeads[i] = Object2D()
		  note = holdHeads[i]
		  note.Image = "Down Hold Head Active.png"
		  setNoteStuff(note, i, rotTable)
		end

	end

	function drawHoldTailInternal(lane, loc, frac, active_level)
		local note;
		note = holdTailsInactive[lane + 1]
		
		if active_level == 2 then
			note = holdTailsActive[lane + 1]
		note.LightenFactor = 1
	  else
		note.LightenFactor = 0
		end
		
		if Game:IsUpscrolling() then
			note.Y = loc + NoteHeight / 2
			note.Rotation = 0
		else 
			note.Y = loc - NoteHeight / 2
			note.Rotation = 180
		end
		
		if active_level ~= 3 then
			Render(note)
		end
	end

	function Update(delta, beat)
	end 

	function drawNormalInternal(lane, loc, frac, active_level)
	  local frame = math.floor(Game:GetCurrentBeat() * 4) % 4 + 1
		local note = normalNotes[lane + 1]
	  local yvalue = yTable[frac] or yTable[48]
	  local xvalue = xTable[frame] or xTable[1]
		note.Y = loc
		
		-- colorize note
		note:SetCropByPixels(xvalue.Start, xvalue.End, yvalue.Start, yvalue.End)
		if active_level ~= 3 then
			Render(note)
		end
	end

	-- 1 is enabled. 2 is being pressed. 0 is failed. 3 is succesful hit.
	function drawHoldBodyInternal(lane, loc, size, active_level)
		function do_draw(lane, loc, size, active_level)
			local note = holdBodiesInactive[lane + 1];
		
			if active_level ~= 0 then  
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
				Render(note)
			end
		end
		
		do_draw(lane, loc, size, active_level)
	end

	function drawHoldHeadInternal(lane, loc, frac, active_level)
	  
	  if active_level == 2 then
		local note = holdHeads[lane + 1]
		note.Y = loc
		if active_level ~= 3 then
			Render(note)
		end
	  else
		drawNormalInternal(lane, loc, frac, active_level)
	  end
	end

	function drawMineInternal(lane, loc, frac)
		-- stub while mines are accounted in the scoring system.
	end

	-- From now on, only engine variables are being set.
	-- Barline
	BarlineEnabled = 0
	BarlineOffset = NoteHeight / 2
	BarlineStartX = GearStartX
	BarlineWidth = Noteskin[Lanes].BarlineWidth
	JudgmentLineY = Noteskin[Lanes].GearHeight
	DecreaseHoldSizeWhenBeingHit = 1
	DanglingHeads = 0

	-- How many extra units do you require so that the whole bounding box is accounted
	-- when determining whether to show this note or not.
	NoteScreenSize = NoteHeight / 2

	DrawNormal = drawNormalInternal
	DrawFake = drawNormalInternal
	DrawLift = drawNormalInternal 
	DrawMine = drawMineInternal

	DrawHoldHead = drawHoldHeadInternal
	DrawHoldTail = drawHoldTailInternal
	DrawHoldBody = drawHoldBodyInternal
end

if Lanes == 4 then
	skin_require("custom_defs")
	doMidiNote()
else
	fallback_require("noteskin")
end
