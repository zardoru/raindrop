
ProgressTick = { Image = "VSRG/progress_tick.png" }
Pulse = { Image = "VSRG/pulse_ver.png", Height = 100 }
Jambar = { ImageFG = "VSRG/jam_bar.png" }
MissHighlight = {}

function ProgressTick.Init()
	ProgressTick.Object = Engine:CreateObject()

	-- When not active, Beat <= 0.
	ProgressTick.BeatOffs = 0

	ProgressTick.Object.Image = ProgressTick.Image
	ProgressTick.Object.X = GearStartX - 20
	ProgressTick.Object.Layer = 18
end

function ProgressTick.Run(Delta)
	if Active ~= 0 then
    local dur = SongDurationBeats
		local Ratio = (Beat + ProgressTick.BeatOffs) / dur
		if SongTime > 0 then
			ProgressTick.Object.Alpha = 1
			ProgressTick.Object.Y = Ratio * (ScreenHeight - 16)
		else
			ProgressTick.Object.Alpha = 1 - SongTime / -1.5
			ProgressTick.Object.Y = (ScreenHeight - 16) * math.pow(SongTime / -1.5, 2)
		end
	else
		ProgressTick.Object.Alpha = 0
	end
end

function Pulse.Init()
	Pulse.Object = Engine:CreateObject()

	Pulse.Object.Image = Pulse.Image
	Pulse.Object.BlendMode = BlendAdd

	if Upscroll ~= 0 then
		Pulse.Object.Rotation = (180)
		Pulse.Object.X = GearStartX + GearWidth
		Pulse.Object.Y = JudgmentLineY + Pulse.Height
	else
		Pulse.Object.X = GearStartX
		Pulse.Object.Y = JudgmentLineY - Pulse.Height
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
		MissHighlight[i].Image = "VSRG/miss_highlight.png"
		MissHighlight[i].X = Noteskin[Channels]["Key" .. i .. "X"]
		MissHighlight[i].Y = ScreenHeight/2
		MissHighlight[i].Width = Noteskin[Channels]["Key" .. i .. "Width"]
		MissHighlight[i].Height = ScreenHeight
		MissHighlight[i].Alpha = 0
		MissHighlight[i].Layer = 15
		
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
		MissHighlight.Time[Lane] = math.min(CurrentSPB / 4, 1)
	end
end

function Jambar.Init()
  Jambar.Width = Lifebar.Margin.Width
  Jambar.Height = 335

	Jambar.BarFG = Engine:CreateObject()
	
	Jambar.BarFG.Image = Jambar.ImageFG;
	
	Jambar.BarFG.Centered = 1
	Jambar.BarFG.X = Lifebar.Margin.X
  Jambar.BarFG.Layer = Lifebar.Margin.Layer
	Jambar.BarFG.Width = Jambar.Width
  Jambar.BarFG.Height = Jambar.Height
  Jambar.BarFG.Lighten = 1
end

function Jambar.Run(Delta)
  local targetRem
  if ScoreKeeper:usesO2() == false then
    targetRem = ScoreKeeper:getTotalNotes() / ScoreKeeper:getMaxNotes()
  else
    targetRem = 1 - (15 - (ScoreKeeper:getCoolCombo() % 15 + 1)) / 15
  end
	
  -- Percentage from 0 to 1 of cool combo

  Jambar.BarFG.LightenFactor = (1 - (Beat - math.floor(Beat)))
	
  local Offset = targetRem * Jambar.Height
  Jambar.BarFG.ScaleY = targetRem
  Jambar.BarFG.Y = ScreenHeight - Offset / 2
  Jambar.BarFG:SetCropByPixels( 0, Jambar.Width, Jambar.BarFG.Height - Offset, Jambar.BarFG.Height )
end
