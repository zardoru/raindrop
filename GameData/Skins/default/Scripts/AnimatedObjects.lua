
ProgressTick = {
	Image = "VSRG/progress_tick.png"
}

Pulse = {
	Image = "VSRG/pulse_ver.png",
	Height = 100,
	Width = GearWidth
}

Jambar = {
	ImageFG = "VSRG/jam_bar.png"
}

MissHighlight = {
	KeyInfo = Noteskin[Channels]
}

function ProgressTick:Init()
	self.Object = ScreenObject {
		Image = self.Image,
		Layer = 18
	}

	if self.Transformation then
		self.Object.ChainTransformation = self.Transformation
	end
end

librd.make_new(ProgressTick, ProgressTick.Init)

function ProgressTick:Run(Delta)
	if Active ~= 0 then
    local dur = SongDurationBeats
		local Ratio = Beat / dur
		if SongTime > 0 then
			self.Object.Alpha = 1
			self.Object.Y = Ratio * (ScreenHeight - 16)
		else
			self.Object.Alpha = 1 - SongTime / -1.5
			self.Object.Y = (ScreenHeight - 16) * math.pow(SongTime / -1.5, 2)
		end
	else
		self.Object.Alpha = 0
	end
end

function Pulse:Init()
	self.Object = Engine:CreateObject()

	self.Object.Image = self.Image
	with(self.Object, {
		BlendMode = BlendAdd,
		Layer = 11,
		Alpha = 0,
	})

	if self.Transformation then
		self.Object.Transformation = self.Transformation
	end
end

rdlib.make_new(Pulse, Pulse.init)

function Pulse:Run(Delta)
	if Active ~= 0 then
		local BeatNth = 2
		local BeatMultiplied = Beat * BeatNth
		local NthOfBeat = 1

		if math.floor(BeatMultiplied) % BeatNth == 0 then
			NthOfBeat = BeatMultiplied - math.floor(BeatMultiplied)
		end

		self.Object.Alpha = 1 - NthOfBeat
	else
		self.Object.Alpha = 0
	end
end

function MissHighlight:Init()
	self.Time = {}
	self.CurrentTime = {}

	for i=1, Channels do
		self[i] = ScreenObject {
			Centered = 1
			Image = "VSRG/miss_highlight.png"
			X = self.KeyInfo["Key" .. i .. "X"]
			Y = ScreenHeight/2
			Alpha = 0
			Layer = 15
		}

		self[i].Width = self.KeyInfo["Key" .. i .. "Width"]
		self[i].Height = ScreenHeight

		self.Time[i] = 1
		self.CurrentTime[i] = 1
	end
end

librd.make_new(MissHighlight, MissHighlight.Init)

function MissHighlight:Run(Delta)
	for i=1, Channels do
		self.CurrentTime[i] = self.CurrentTime[i] + Delta
		self[i].Alpha = clerp(self.CurrentTime[i], 0, self.CurrentTime[i], 1, 0)
	end
end

function MissHighlight:Miss(t, l, h)
	self.CurrentTime[l] = 0

	if CurrentSPB ~= math.huge then
		self.Time[l] = math.min(CurrentSPB / 4, 0.5)
	end
end

function Jambar:Init()
  self.Width = Lifebar.Margin.Width
  self.Height = 335

	self.BarFG = Engine:CreateObject()
	self.BarFG.Image = self.ImageFG;
	with (self.BarFG, {
		Centered = 1,
		X = Lifebar.Margin.X,
	  Layer = Lifebar.Margin.Layer,
		Width = Jambar.Width,
	  Height = Jambar.Height,
	  Lighten = 1
	})
end

librd.make_new(Jambar, Jambar.Init)

function Jambar:Run(Delta)
  local targetRem
  if ScoreKeeper:usesO2() == false then
    targetRem = ScoreKeeper:getTotalNotes() / ScoreKeeper:getMaxNotes()
  else
    targetRem = 1 - (15 - (ScoreKeeper:getCoolCombo() % 15 + 1)) / 15
  end

  -- Percentage from 0 to 1 of cool combo

  self.BarFG.LightenFactor = (1 - (Beat - math.floor(Beat)))

  local Offset = targetRem * Jambar.Height
  self.BarFG.ScaleY = targetRem
  self.BarFG.Y = ScreenHeight - Offset / 2
  self.BarFG:SetCropByPixels( 0, self.Width, self.BarFG.Height - Offset, self.BarFG.Height )
end
