ScoreDisplay = ScoreDisplay or {}

with(ScoreDisplay, {
    DigitWidth = 30,
    Sheet = "VSRG/combosheet.csv",
    DigitHeight = 30,
    DigitCount = 9,
})

with(ScoreDisplay, {
    W = ScoreDisplay.DigitWidth * ScoreDisplay.DigitCount,
    H = ScoreDisplay.DigitHeight
})

with(ScoreDisplay, {
    X = ScreenWidth - ScoreDisplay.W, -- Topleft
    Y = ScreenHeight - ScoreDisplay.H,
    Layer = 20
})

function ScoreDisplay:SetName(i)
	return i-1 .. ".png"
end

function ScoreDisplay:Init()

  self.Targets = {}
  self.Images = {}

  self.Digits = {}

	self.Score = 0
	self.DisplayScore = 0

	for i = 1, 10 do -- Digit images
		self.Images[i] = self:SetName(i)
	end

	self.Atlas = TextureAtlas:new(GetSkinFile(self.Sheet))

	for i = 1, self.DigitCount do
		self.Targets[i] = Engine:CreateObject()

		self.Targets[i].X = self.X + self.W - self.DigitWidth * i
		self.Targets[i].Y = self.Y + self.H - self.DigitHeight
		self.Targets[i].Texture = self.Atlas.File
		self.Targets[i].Width = self.DigitWidth
		self.Targets[i].Height = self.DigitHeight
		self.Targets[i].Layer = self.Layer

		local Tab = self.Atlas.Sprites[self.Images[1]]

		self.Targets[i]:SetCropByPixels(Tab.x, Tab.x+Tab.w, Tab.y+Tab.h, Tab.y)

		self.Targets[i].Alpha = (1)
	end
end

librd.make_new(ScoreDisplay, ScoreDisplay.Init)

function ScoreDisplay:Run(Delta)
  self.Score = self.Player.Score
  
	self.DisplayScore = math.min((self.Score - self.DisplayScore) * Delta * 40 + self.DisplayScore, self.Score)
	local Digits = librd.intToDigits(self.DisplayScore)
  local tdig = #Digits

	for i=1, tdig do
		local Tab = self.Atlas.Sprites[self.Images[Digits[tdig - (i - 1)]+1]]
		self.Targets[i]:SetCropByPixels(Tab.x, Tab.x+Tab.w, Tab.y, Tab.y+Tab.h)
		self.Targets[i].Alpha = (1)
	end

	for i=tdig+1, self.DigitCount do
		local Tab = self.Atlas.Sprites[self.Images[1]]

		self.Targets[i]:SetCropByPixels(Tab.x, Tab.x+Tab.w, Tab.y+Tab.h, Tab.y)
		self.Targets[i].Alpha = (1)
	end

end
