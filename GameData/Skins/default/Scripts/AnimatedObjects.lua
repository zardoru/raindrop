
ProgressTick = {
	Image = "VSRG/progress_tick.png",
  Start = -8,
  End = ScreenHeight
}

Pulse = {
	Image = "VSRG/pulse_ver.png",
	Height = 100,
}



MissHighlight = {
  Image = "VSRG/miss_highlight.png"
}

function ProgressTick:Init()
	self.Object = ScreenObject {
		Texture = self.Image,
		Layer = 18,
    X = self.Noteskin.GearStartX - 16
	}
  
  if self.Transformation then
		self.Object.ChainTransformation = self.Transformation
  end
end

librd.make_new(ProgressTick, ProgressTick.Init)

function ProgressTick:Run(Delta)
	if Game.Active then
    local dur = self.Player.BeatDuration
		local Ratio = self.Player.Beat / dur
		if self.Player.Time > 0 then
			self.Object.Alpha = 1
			self.Object.Y = cmix(Ratio, self.Start, self.End)
		else
			self.Object.Alpha = 1 - self.Player.Time / -1.5
			self.Object.Y = cmix(math.pow(self.Player.Time / -1.5, 2), self.Start, self.End)
		end
	else
		self.Object.Alpha = 0
	end
end

function Pulse:Init()
	self.Object = Engine:CreateObject()

	self.Object.Texture = self.Image
	with(self.Object, {
		BlendMode = BlendAdd,
		Layer = 11,
		Alpha = 0,
    })
    self.Object.X = self.Noteskin.GearStartX
    self.Object.Y = self.Player.JudgmentY - self.Object.Height

  if self.Transformation then
		self.Object.ChainTransformation = self.Transformation
	end
end

librd.make_new(Pulse, Pulse.Init)

function Pulse:Run(Delta)
  
	if Game.Active ~= 0 then
		local BeatNth = 2
		local BeatMultiplied = self.Player.Beat * BeatNth
		local NthOfBeat = 1

		if floor(BeatMultiplied) % BeatNth == 0 then
			NthOfBeat = fract(BeatMultiplied)
		end

		self.Object.Alpha = 1 - NthOfBeat
	else
		self.Object.Alpha = 0
	end
end

function MissHighlight:Init()
	self.Time = {}
	self.CurrentTime = {}

	for i=1, self.Player.Channels do
		self[i] = ScreenObject {
			Centered = 1,
			Texture = self.Image,
			X = self.Noteskin["Key" .. i .. "X"],
			Y = ScreenHeight / 2,
			Alpha = 0,
			Layer = 15
		}

		self[i].Width = self.Noteskin["Key" .. i .. "Width"]
		self[i].Height = ScreenHeight

		self.Time[i] = 1
		self.CurrentTime[i] = 1
	end
  
  if self.Transformation then
			self[i].ChainTransformation = self.Transformation
		end
end

librd.make_new(MissHighlight, MissHighlight.Init)

function MissHighlight:Run(Delta)
	for i=1, self.Player.Channels do
		self.CurrentTime[i] = self.CurrentTime[i] + Delta
		self[i].Alpha = clerp(self.CurrentTime[i], 0, self.Time[i], 1, 0)
	end
end

function MissHighlight:Miss(t, l, h, pn)
  if pn ~= self.Player.Number then
    return
  end
  
	self.CurrentTime[l] = 0
  local spb = 60 / self.Player.BPM

	if spb ~= math.huge then
		self.Time[l] = math.min(spb / 4, 0.5)
	end
end

