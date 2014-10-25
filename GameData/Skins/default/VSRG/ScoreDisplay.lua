ScoreDisplay = {}

ScoreDisplay.Digits = {}
ScoreDisplay.DigitWidth = 30
ScoreDisplay.DigitHeight = 30
ScoreDisplay.DigitCount = 9
ScoreDisplay.Targets = {}
ScoreDisplay.Images = {}

function ScoreDisplay.Init()

	ScoreDisplay.Score = 0
	ScoreDisplay.DisplayScore = 0

	for i = 1, 10 do -- Digit images
		ScoreDisplay.Images[i] = i-1 .. ".png"
	end

	ScoreDisplay.Atlas = TextureAtlas:new(Obj.GetSkinDirectory() .. "VSRG/combosheet.csv")

	for i = 1, ScoreDisplay.DigitCount do
		ScoreDisplay.Targets[i] = Obj.CreateTarget()
		Obj.SetTarget(ScoreDisplay.Targets[i])
		Obj.SetPosition(ScreenWidth - ScoreDisplay.DigitWidth * i, ScreenHeight - ScoreDisplay.DigitHeight)
		Obj.SetImageSkin(ScoreDisplay.Atlas.File)
		Obj.SetSize(ScoreDisplay.DigitWidth, ScoreDisplay.DigitHeight)

		local Tab = ScoreDisplay.Atlas.Sprites[ScoreDisplay.Images[1]]

		Obj.CropByPixels(Tab.x, Tab.y, Tab.x+Tab.w, Tab.y+Tab.h)

		Obj.SetAlpha(1)
	end
end

function ScoreDisplay.Run(Delta)
	ScoreDisplay.DisplayScore = math.min((ScoreDisplay.Score - ScoreDisplay.DisplayScore) * Delta * 100 + ScoreDisplay.DisplayScore, ScoreDisplay.Score)

	ScoreDisplay.Digits = {}

	local TCombo = math.floor(ScoreDisplay.DisplayScore)
	local tdig = 0

	while TCombo >= 1 do
		table.insert(ScoreDisplay.Digits, math.floor(TCombo) % 10)
		TCombo = TCombo / 10
		tdig = tdig + 1
	end

	for i=1, tdig do
		Obj.SetTarget(ScoreDisplay.Targets[i])

		local Tab = ScoreDisplay.Atlas.Sprites[ScoreDisplay.Images[ScoreDisplay.Digits[i]+1]]
		Obj.CropByPixels(Tab.x, Tab.y, Tab.x+Tab.w, Tab.y+Tab.h)
		Obj.SetSize(ScoreDisplay.DigitWidth, ScoreDisplay.DigitHeight)
		Obj.SetAlpha(1)

	end

	for i=tdig+1, ScoreDisplay.DigitCount do

		Obj.SetTarget(ScoreDisplay.Targets[i])

		local Tab = ScoreDisplay.Atlas.Sprites[ScoreDisplay.Images[1]]

		Obj.CropByPixels(Tab.x, Tab.y, Tab.x+Tab.w, Tab.y+Tab.h)
		Obj.SetSize(ScoreDisplay.DigitWidth, ScoreDisplay.DigitHeight)
		Obj.SetAlpha(1)
	end

end

function ScoreDisplay.Update()
	ScoreDisplay.Score = SCScore
end

function ScoreDisplay.Cleanup()

	for i = 1, ScoreDisplay.DigitCount do
		Obj.CleanTarget(ScoreDisplay.Targets[i])
	end

end
