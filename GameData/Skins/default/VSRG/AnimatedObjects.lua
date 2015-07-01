
ProgressTick = { Image = "VSRG/progress_tick.png" }
Pulse = { Image = "VSRG/pulse_ver.png", Height = 100 }
Jambar = { ImageBG = "VSRG/jfill_bg.jpg", ImageFG = "VSRG/jfill_fg.jpg", Width = 512 }
MissHighlight = {}

function ProgressTick.Init()
	ProgressTick.Object = Engine:CreateObject()

	-- When not active, Beat <= 0.
	ProgressTick.BeatOffs = -Beat

	ProgressTick.Object.Image = ProgressTick.Image
	ProgressTick.X = GearStartX - 5 - 16
	ProgressTick.Layer = 25
end

function ProgressTick.Run(Delta)
	if Active ~= 0 then
		local Ratio = (Beat + ProgressTick.BeatOffs) / (SongDurationBeats + ProgressTick.BeatOffs)
		if SongTime > 0 then
			ProgressTick.Alpha = 1
			ProgressTick.Y = Ratio * (ScreenHeight - 16)
		else
			ProgressTick.Alpha = 1 - SongTime / -1.5
			ProgressTick.Y = (ScreenHeight - 16) * math.pow(SongTime / -1.5, 2)
		end
	else
		ProgressTick.Alpha = 0
	end
end

function Pulse.Init()
	Pulse.Object = Engine:CreateObject()

	Pulse.Object.Image = Pulse.Image
	Pulse.Object.BlendMode = BlendAdd

	if Upscroll ~= 0 then
		Pulse.Object.Rotation = (180)
		Pulse.Object.X = GearStartX + GearWidth
		Pulse.Object.Y = GearHeight + Pulse.Height
	else
		Pulse.Object.X = GearStartX
		Pulse.Object.Y = ScreenHeight - GearHeight - Pulse.Height
	end

	Pulse.Object.Width = GearWidth
	Pulse.Object.Height = Pulse.Height
	Pulse.Object.Layer = 11
	Pulse.Object.Alpha = 0
end

function Pulse.Run(Delta)
	if Active ~= 0 then
		local BeatNth = 2
		local BeatMultiplied = Beat * BeatNth
		local NthOfBeat = 1
		
		if math.floor(BeatMultiplied) % BeatNth == 0 then
			NthOfBeat = BeatMultiplied - math.floor(BeatMultiplied)
		end
		
		Pulse.Object.Alpha = (1 - NthOfBeat)
	else
		Pulse.Object.Alpha = 0
	end
end

function MissHighlight.Init()

	MissHighlight.Time = {}
	MissHighlight.CurrentTime = {}

	for i=1, Channels do
		MissHighlight[i] = Engine:CreateObject()
		MissHighlight[i].Centered = 1
		MissHighlight[i].Image = ("VSRG/miss_highlight.png")
		print ("got here lol " .. i)
		MissHighlight[i].X = Noteskin[Channels]["Key" .. i .. "X"]
		MissHighlight[i].Y = ScreenHeight/2
		print "got here ye"
		MissHighlight[i].Width = Noteskin[Channels]["Key" .. i .. "Width"]
		MissHighlight[i].Height = ScreenHeight
		MissHighlight[i].Alpha = 0
		MissHighlight[i].Layer = 15
		
		print "got here"
		MissHighlight.Time[i] = 1 
		MissHighlight.CurrentTime[i] = 1
	end
end

function MissHighlight.Run(Delta)
	for i=1, Channels do
		MissHighlight.CurrentTime[i] = MissHighlight.CurrentTime[i] + Delta
		
		if MissHighlight.CurrentTime[i] < MissHighlight.Time[i] then
			local Lerp = 1 - MissHighlight.CurrentTime[i] / MissHighlight.Time[i]
			MissHighlight[i].Alpha = Lerp
		else
			MissHighlight[i].Alpha = 0
		end
	end
end

function MissHighlight.OnMiss(Lane)
	MissHighlight.CurrentTime[Lane] = 0

	if CurrentSPB ~= math.huge then
		MissHighlight.Time[Lane] = CurrentSPB / 4
	end
end

function Jambar.Init()
	if ScoreKeeper:usesO2() == false then
		return
	end

	Jambar.BarBG = Engine:CreateObject()
	Jambar.BarFG = Engine:CreateObject()
	
	Jambar.BarBG.Image = Jambar.ImageBG;
	Jambar.BarFG.Image = Jambar.ImageFG;
	
	Jambar.BarBG.Z = 20
	Jambar.BarFG.Z = 21
	
	Jambar.BarBG.Centered = 1
	Jambar.BarFG.Centered = 1
	Jambar.BarBG.X = ScreenWidth / 2
	Jambar.BarFG.X = ScreenWidth / 2
	
	Jambar.BarBG.Y = ScreenHeight - Jambar.BarBG.Height
	Jambar.BarFG.Y = ScreenHeight - Jambar.BarFG.Height
		
	Jambar.OldRem = 0
end

function Jambar.Run(Delta)
	if ScoreKeeper:usesO2() == false then
		return
	end
	
	-- Percentage from 0 to 1 of cool combo
	local targetRem = 1 - (15 - ScoreKeeper:getCoolCombo() % 15) / 15
	local deltaRem = (targetRem - Jambar.OldRem) * Delta * 50
	
	if deltaRem < 0 then 
		deltaRem = (targetRem - Jambar.OldRem) -- Jump to it.
	end
	
	Jambar.BarFG.Width = Jambar.OldRem * Jambar.Width + deltaRem * Jambar.Width
	Jambar.BarFG.Lighten = true
	Jambar.BarFG.LightenFactor = (1 - (Beat - math.floor(Beat))) * 0.5
	
	local Center = Jambar.Width / 2
	local Offset = Jambar.OldRem * Jambar.Width / 2 + deltaRem * Jambar.Width / 2
	Jambar.BarFG:SetCropByPixels( Center - Offset, Center + Offset, 0, Jambar.BarFG.Height )
	Jambar.OldRem = Jambar.OldRem + deltaRem
end
