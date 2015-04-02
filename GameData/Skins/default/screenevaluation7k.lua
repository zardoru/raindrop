game_require("textureatlas.lua")
skin_require ("Global/Background.lua")
skin_require ("Global/FadeInScreen.lua")
skin_require ("VSRG/ScoreDisplay.lua")

function SetupFonts()
	EvalFont = Fonts.TruetypeFont(GetSkinFile("font.ttf"), 30);
end

function GetRankImage()
	scorerank = ScoreKeeper:getBMRank()
	if scorerank == PMT_AAA then
		return "AAA"
	elseif scorerank == PMT_AA then
		return "AA"
	elseif scorerank == PMT_A then
		return "A"
	elseif scorerank == PMT_B then
		return "B"
	elseif scorerank == PMT_C then
		return "C"
	elseif scorerank == PMT_D then
		return "D"
	else
		return "F"
	end
	
end

function SetupRank()
	RankPic = Engine:CreateObject()
	RankPic.Image = "Evaluation/score" .. GetRankImage() .. ".png"
	RankPic.Centered = 1
	RankPic.X = ScreenWidth / 2 - RankPic.Width / 2
	RankPic.Y = ScreenHeight / 2
	
	-- resize to something acceptable
	if RankPic.X < RankPic.Width / 2 then
		local Rat = (ScreenWidth / 2) / (RankPic.Width)
		RankPic:SetScale(Rat)
		RankPic.X = ScreenWidth / 2 - RankPic.Width / 2 * Rat
	end
	
	RankStr = StringObject2D()
	RankStr.Text = "rank"
	RankStr.X = RankPic.X - EvalFont:GetLength("rank") / 2
	RankStr.Y = ScreenHeight / 2 + RankPic.Height / 2 + 10
	RankStr.Font = EvalFont
	
	Engine:AddTarget(RankStr)
end


function SetupJudgmentsDisplay()
	JudgeStr = StringObject2D()
	JudgeStr.Font = EvalFont
	ScoreKeeper = Global:GetScorekeeper7K()
	
	w0 = ScoreKeeper:getJudgmentCount(SKJ_W0)
	w1 = ScoreKeeper:getJudgmentCount(SKJ_W1)
	w2 = ScoreKeeper:getJudgmentCount(SKJ_W2)
	w3 = ScoreKeeper:getJudgmentCount(SKJ_W3)
	w4 = ScoreKeeper:getJudgmentCount(SKJ_W4)
	w5 = ScoreKeeper:getJudgmentCount(SKJ_MISS)
	
	fmtext = ""
	if ScoreKeeper:usesW0() == false then
		if ScoreKeeper:usesO2() == false then
			Score = Global:GetScorekeeper7K():getScore(ST_EXP3)
			fmtext = fmtext .. string.format("Flawless: %04d\nSweet: %04d\nNice: %04d\nWeak: %04d\nMiss: %04d", w1, w2, w3, w4, w5)
		else
			local p = ScoreKeeper:getPills()
			Score = Global:GetScorekeeper7K():getScore(ST_O2JAM)
			fmtext = fmtext .. string.format("Flawless: %04d\nSweet: %04d\nNice: %04d\nMiss: %04d", w1, w2, w3, w5, p, rem)
		end
	else
		Score = Global:GetScorekeeper7K():getScore(ST_EXP3)
		fmtext = fmtext .. string.format("Flawless*: %04d\nFlawless: %04d\nSweet: %04d\nNice: %04d\nOK: %04d\nMiss: %04d", w0, w1, w2, w3, w4, w5)
	end
	
	print (ScoreDisplay.Score)
	fmtext = fmtext .. string.format("\nMax Combo: %d", ScoreKeeper:getScore(ST_MAX_COMBO))
	fmtext = fmtext .. string.format("\nNotes hit: %d%%", ScoreKeeper:getPercentScore(PST_NH))
	fmtext = fmtext .. string.format("\nAccuracy: %d%%", ScoreKeeper:getPercentScore(PST_ACC))
	fmtext = fmtext .. "\nraindrop rank: "
	
	if ScoreKeeper:getRank() > 0 then
		fmtext = fmtext .. "+" .. ScoreKeeper:getRank()
	else
		fmtext = fmtext .. ScoreKeeper:getRank()
	end
	
	JudgeStr.Text = fmtext
	
	ScoreDisplay.X = ScoreDisplay.W / 2 + ScreenWidth / 2
	ScoreDisplay.Y = RankStr.Y + 30

	JudgeStr.X = ScoreDisplay.X
	JudgeStr.Y = ScreenHeight / 2 - RankPic.Height / 2
	
	Engine:AddTarget(JudgeStr)
end

function SetSongTitle()
	Filter = Engine:CreateObject()
	Filter.Image = "Global/filter.png"
	Filter.X = 0
	Filter.Y = ScreenHeight - 30
	Filter.Width = ScreenWidth
	Filter.Height = 40
	
	TitleText = StringObject2D()
	
	sng = toSong7K(Global:GetSelectedSong())
	diff = sng:GetDifficulty(Global.DifficultyIndex)
	if diff.Author ~= "" then
		difftxt = string.format("%s by %s", diff.Name, diff.Author)
	else
		difftxt = string.format("%s", diff.Name)
	end
	
	local Text = string.format ("%s by %s (Chart: %s)", Global:GetSelectedSong().Title, Global:GetSelectedSong().Author, difftxt)
	TitleText.Text = Text
	TitleText.Font = EvalFont
	
	TitleText.Y = ScreenHeight - 40
	TitleText.X = ScreenWidth / 2 - EvalFont:GetLength(Text) / 2
	
	Engine:AddTarget(TitleText)
end

function Init()
	
	BackgroundAnimation:Init()
	SetupFonts()
	SetupRank()
	SetupJudgmentsDisplay()
	
	ScoreDisplay.Init()
	ScoreDisplay.Score = Score
	
	scoreStr = StringObject2D()
	scoreStr.Font = EvalFont
	scoreStr.X = ScoreDisplay.X
	scoreStr.Y = ScoreDisplay.Y + ScoreDisplay.H
	scoreStr.Text = "score"
	
	Engine:AddTarget(scoreStr)
	
	SetSongTitle()
	ScreenFade.Init()
	ScreenFade.Out()
	
end

function Cleanup()

end

function Update(Delta)
	ScoreDisplay.Run(Delta)
end
