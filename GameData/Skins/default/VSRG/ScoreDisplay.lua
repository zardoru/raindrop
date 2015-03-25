ScoreDisplay = ScoreDisplay or {}

ScoreDisplay.Digits = {}
ScoreDisplay.DigitWidth = 30
ScoreDisplay.Sheet = "VSRG/combosheet.csv"
ScoreDisplay.DigitHeight = 30
ScoreDisplay.DigitCount = 9
ScoreDisplay.W = ScoreDisplay.DigitWidth * ScoreDisplay.DigitCount
ScoreDisplay.H = ScoreDisplay.DigitHeight
ScoreDisplay.X = ScreenWidth - ScoreDisplay.W -- Topleft
ScoreDisplay.Y = ScreenHeight - ScoreDisplay.H
ScoreDisplay.Targets = {}
ScoreDisplay.Images = {}

function ScoreDisplay.SetName(i)
	return i-1 .. ".png"
end

function ScoreDisplay.Init()

	ScoreDisplay.Score = 0
	ScoreDisplay.DisplayScore = 0

	for i = 1, 10 do -- Digit images
		ScoreDisplay.Images[i] = ScoreDisplay.SetName(i)
	end

	ScoreDisplay.Atlas = TextureAtlas:new(GetSkinFile(ScoreDisplay.Sheet))

	for i = 1, ScoreDisplay.DigitCount do
		ScoreDisplay.Targets[i] = Engine:CreateObject()

		ScoreDisplay.Targets[i].X = ScoreDisplay.X + ScoreDisplay.W - ScoreDisplay.DigitWidth * i
		ScoreDisplay.Targets[i].Y = ScoreDisplay.Y + ScoreDisplay.H - ScoreDisplay.DigitHeight
		ScoreDisplay.Targets[i].Image = ("VSRG/"..ScoreDisplay.Atlas.File)
		ScoreDisplay.Targets[i].Width = ScoreDisplay.DigitWidth
		ScoreDisplay.Targets[i].Height = ScoreDisplay.DigitHeight

		local Tab = ScoreDisplay.Atlas.Sprites[ScoreDisplay.Images[1]]

		ScoreDisplay.Targets[i]:SetCropByPixels(Tab.x, Tab.x+Tab.w, Tab.y+Tab.h, Tab.y)

		ScoreDisplay.Targets[i].Alpha = (1)
	end
end

function ScoreDisplay.Run(Delta)
	ScoreDisplay.DisplayScore = math.min((ScoreDisplay.Score - ScoreDisplay.DisplayScore) * Delta * 40 + ScoreDisplay.DisplayScore, ScoreDisplay.Score)
	ScoreDisplay.Digits = {}

	local TCombo = math.ceil(ScoreDisplay.DisplayScore)
	local tdig = 0

	while TCombo >= 1 do
		table.insert(ScoreDisplay.Digits, math.floor(TCombo) % 10)
		TCombo = TCombo / 10
		tdig = tdig + 1
	end

	for i=1, tdig do
		local Tab = ScoreDisplay.Atlas.Sprites[ScoreDisplay.Images[ScoreDisplay.Digits[i]+1]]
		ScoreDisplay.Targets[i]:SetCropByPixels(Tab.x, Tab.x+Tab.w, Tab.y, Tab.y+Tab.h)
		ScoreDisplay.Targets[i].Alpha = (1)
	end

	for i=tdig+1, ScoreDisplay.DigitCount do
		local Tab = ScoreDisplay.Atlas.Sprites[ScoreDisplay.Images[1]]

		ScoreDisplay.Targets[i]:SetCropByPixels(Tab.x, Tab.x+Tab.w, Tab.y+Tab.h, Tab.y)
		ScoreDisplay.Targets[i].Alpha = (1)
	end

end

function ScoreDisplay.Update()
	ScoreDisplay.Score = SCScore
end