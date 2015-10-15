Judgment = {
	FadeoutTime = 0.5,
	FadeoutDuration = 0.15,
	Speed = 25,
	Tilt = 7, -- in degrees

	Scale = 0.20,
	ScaleHit = 0.08,
	ScaleMiss = 0.12,
	ScaleExtra = 0.1,

	Position = {
		x = GearWidth/2 + GearStartX,
		y = ComboDisplay.Position.y + ComboDisplay.DigitHeight + 20
	},

	Table = {
		"judge-excellent.png",
		"judge-perfect.png",
		"judge-good.png",
		"judge-bad.png",
		"judge-miss.png",
	},

	TimingIndicator = "hiterror.png",
	ShowTimingIndicator = 1
}

function Judgment.Init()
	Judgment.Atlas = TextureAtlas:new(GetSkinFile("VSRG/judgment.csv"))
	Judgment.Object = Engine:CreateObject()
	
	Judgment.Object.Layer = 24
	Judgment.Object.Centered = 1
	Judgment.Object:SetScale(Judgment.Scale)
	Judgment.Object.Image = Judgment.Atlas.File
	Judgment.Object.X = Judgment.Position.x
	Judgment.Object.Y = Judgment.Position.y
	Judgment.Object.Alpha = 0
	
	Judgment.LastAlternation = 0
	Judgment.Time = Judgment.FadeoutTime + Judgment.FadeoutDuration

	Judgment.IndicatorObject = Engine:CreateObject()
	
	Judgment.IndicatorObject.Image = ("VSRG/" .. Judgment.TimingIndicator)
	Judgment.IndicatorObject.Layer = 24
	Judgment.IndicatorObject.Centered = 1
	Judgment.IndicatorObject:SetScale(Judgment.Scale)
	Judgment.IndicatorObject.Alpha = 0
	Judgment.IndicatorObject.Y = Judgment.Object.Y
end

function GetComboLerp()
	local ComboPercent = ScoreKeeper:getScore(ST_COMBO) / ScoreKeeper:getMaxNotes()
	local AAAThreshold = 8.0 / 9.0
	local ComboLerp = math.min(ComboPercent, AAAThreshold) / AAAThreshold
	return ComboLerp
end

function Judgment.Run(Delta)
	local ComboLerp = GetComboLerp()
	
	if Active ~= 0 then
		local AlphaRatio
		Judgment.Time = Judgment.Time + Delta

		local OldJudgeScale = Judgment.Object.ScaleX
		local ScaleLerpAAA = ComboLerp * Judgment.ScaleExtra
		local DeltaScale = (Judgment.Scale + ScaleLerpAAA - OldJudgeScale) * Delta * Judgment.Speed
		local FinalScale = math.max(0, OldJudgeScale + DeltaScale)

		Judgment.Object:SetScale (FinalScale)

		if Judgment.Time > Judgment.FadeoutTime then
			local Time = Judgment.Time - Judgment.FadeoutTime
			if Time > 0 then
				local Ratio = Time / Judgment.FadeoutDuration

				if Ratio < 1 then
					AlphaRatio = 1 - Ratio
				else
					AlphaRatio = 0
				end

			end
		else
			AlphaRatio = 1
		end
		
		local w = Judgment.Object.Width
		local h = Judgment.Object.Height

		Judgment.Object.Alpha = (AlphaRatio)
				
		if Judgment.LastAlternation == 0 then
			Judgment.Object.Rotation = (Judgment.Tilt * ComboLerp)
		else
			Judgment.Object.Rotation = (-Judgment.Tilt * ComboLerp)
		end

		if Judgment.Value ~= 1 and Judgment.ShowTimingIndicator == 1 then -- not a "flawless"
			
			Judgment.IndicatorObject.Alpha = (AlphaRatio)

			local NewRatio = FinalScale / Judgment.Scale
			Judgment.IndicatorObject:SetScale(NewRatio)

			if Judgment.EarlyOrLate == 1 then -- early
				Judgment.IndicatorObject.X = Judgment.Position.x - w/2 * FinalScale - 20
			else 
				if Judgment.EarlyOrLate == 2 then -- late
					Judgment.IndicatorObject.X = Judgment.Position.x + w/2 * FinalScale + 20
				end
			end
		else
			Judgment.IndicatorObject.Alpha = 0
		end
	end
end

function Judgment.Hit(JudgmentValue, EarlyOrLate)

	Judgment.Value = JudgmentValue

	if Judgment.Value == 0 then
		Judgment.Object.Lighten = (1)
		Judgment.Object.LightenFactor = (1.5)
		Judgment.Value = 1
	else
		Judgment.Object.Lighten = 0
		Judgment.Object.LightenFactor = 0
	end

	if Judgment.LastAlternation == 0 then
		Judgment.LastAlternation = 1
	else 
		Judgment.LastAlternation = 0
	end
	
	Judgment.Atlas:SetObjectCrop(Judgment.Object, Judgment.Table[Judgment.Value])
	Judgment.Object.Height = Judgment.Atlas.Sprites[Judgment.Table[Judgment.Value]].h
	Judgment.Object.Width = Judgment.Atlas.Sprites[Judgment.Table[Judgment.Value]].w
	
	if JudgmentValue ~= 5 then
		if JudgmentValue ~= -1 then
			local CLerp = GetComboLerp()
			Judgment.Object:SetScale (Judgment.ScaleHit + CLerp * Judgment.ScaleExtra)
		else
			Judgment.Object:SetScale (Judgment.Scale)
		end
	else
		Judgment.Object:SetScale (Judgment.ScaleMiss)
	end

	Judgment.Time = 0
	Judgment.EarlyOrLate = EarlyOrLate
end
