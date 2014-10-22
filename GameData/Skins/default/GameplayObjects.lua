

Lifebar = {
	FillSize = 367,
	MarginFile = "stage-lifeb.png",
	FillFile = "stage-lifeb-s.png",
	Width = 50,
	Speed = 10
}

Judgment = {

	FadeoutTime = 2,
	FadeoutDuration = 0.5,
	Speed = 15,

	Scale = 0.20,
	ScaleHit = 0.30,
	ScaleMiss = 0.12,

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

	TimingIndicator = "hiterror.png"

}

function Lifebar.Init()

	Lifebar.Margin = Obj.CreateTarget()
	Lifebar.Fill = Obj.CreateTarget()
	Lifebar.Fill2 = Obj.CreateTarget()

	Obj.SetTarget(Lifebar.Margin)
	Obj.SetImageSkin(Lifebar.MarginFile)
	Obj.SetZ(25)
	Obj.SetCentered(1)

	w, h = Obj.GetSize()

	Lifebar.Position = { 
		x = GearStartX + GearWidth + 42,
		y = ScreenHeight - h / 2
	}

	Lifebar.CurrentPosition = {
		x = Lifebar.Position.x,
		y = ScreenHeight
	}

	Obj.SetPosition(Lifebar.CurrentPosition.x, Lifebar.CurrentPosition.y)

	Obj.SetTarget(Lifebar.Fill)
	Obj.SetImageSkin(Lifebar.FillFile)
	Obj.SetSize(Lifebar.Width, Lifebar.FillSize)
	Obj.SetZ(26)
	Obj.SetCentered(1)

	Obj.SetTarget(Lifebar.Fill2)
	Obj.SetImageSkin(Lifebar.FillFile)
	Obj.SetZ(26)
	Obj.SetCentered(1)
	Obj.SetBlendMode(0)

	Lifebar.Display = 0

end

function Lifebar.Cleanup()
	Obj.CleanTarget(Lifebar.Margin)
	Obj.CleanTarget(Lifebar.Fill)
	Obj.CleanTarget(Lifebar.Fill2)
end

function Lifebar.Run(Delta)
	local DeltaLifebar = (LifebarValue - Lifebar.Display)
	local DP = 1 - (Beat - math.floor(Beat))

	Lifebar.Display = DeltaLifebar * Delta * Lifebar.Speed + Lifebar.Display

	local partA = Lifebar.Display * 0.98
	local partB = Lifebar.Display * 0.02 * DP
	local Display = partA + partB
	local NewY = ScreenHeight - Lifebar.FillSize * (Display) / 2
	local NewYFixed = ScreenHeight - Lifebar.FillSize * (Lifebar.Display) / 2

	Lifebar.CurrentPosition = Lifebar.Position

	Obj.SetTarget(Lifebar.Fill)
	Obj.SetScale( 1, Lifebar.Display )
	Obj.SetPosition( Lifebar.CurrentPosition.x, NewYFixed );
	Obj.CropByPixels( 0, Lifebar.FillSize - Lifebar.FillSize * Lifebar.Display, Lifebar.Width, Lifebar.FillSize )

	Obj.SetTarget(Lifebar.Fill2)
	Obj.SetScale( 1, Display )
	Obj.SetPosition( Lifebar.CurrentPosition.x, NewY );
	Obj.CropByPixels( 0, Lifebar.FillSize - Lifebar.FillSize * Display, Lifebar.Width, Lifebar.FillSize )
	Obj.SetAlpha ( DP * LifebarValue )

	Obj.SetTarget(Lifebar.Margin)
	Obj.SetPosition( Lifebar.CurrentPosition.x, Lifebar.CurrentPosition.y )
end

function Judgment.Init()
	Judgment.Object = Obj.CreateTarget()
	Obj.SetTarget(Judgment.Object)
	Obj.SetZ(24)
	Obj.SetCentered(1)
	Obj.SetScale (Judgment.Scale, Judgment.Scale)
	Obj.SetPosition (Judgment.Position.x, Judgment.Position.y)

	Judgment.Time = Judgment.FadeoutTime

	Judgment.IndicatorObject = Obj.CreateTarget()
	Obj.SetTarget(Judgment.IndicatorObject)
	Obj.SetImageSkin(Judgment.TimingIndicator)
	Obj.SetZ(24)
	Obj.SetCentered(1)
	Obj.SetScale(Judgment.Scale, Judgment.Scale)
	Obj.SetAlpha(0)
end

function Judgment.Cleanup()
	Obj.CleanTarget(Judgment.Object)
end

function Judgment.Run(Delta)
	if Active ~= 0 then
		local AlphaRatio
		Judgment.Time = Judgment.Time + Delta

		Obj.SetTarget(Judgment.Object)
		local OldJudgeScale = Obj.GetScale()

		local DeltaScale = (Judgment.Scale - OldJudgeScale) * Delta * Judgment.Speed
		local FinalScale = OldJudgeScale + DeltaScale

		Obj.SetScale (FinalScale, FinalScale)

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
		
		local w, h = Obj.GetSize()

		Obj.SetAlpha(AlphaRatio)

		Obj.SetTarget (Judgment.IndicatorObject)

		if Judgment.Value ~= 1 then -- not a "flawless"
			Obj.SetAlpha(AlphaRatio)

			local NewRatio = FinalScale / Judgment.Scale
			Obj.SetScale(NewRatio, NewRatio)

			if Judgment.EarlyOrLate == 1 then -- early
				Obj.SetPosition (Judgment.Position.x - w/2 * FinalScale - 20, Judgment.Position.y)
			else 
				if Judgment.EarlyOrLate == 2 then -- late
					Obj.SetPosition (Judgment.Position.x + w/2 * FinalScale + 20, Judgment.Position.y)
				end
			end
		else
			Obj.SetAlpha(0)
		end
	end
end

function Judgment.Hit(JudgmentValue, EarlyOrLate)
	Obj.SetTarget(Judgment.Object)

	Judgment.Value = JudgmentValue

	if Judgment.Value == 0 then
		Obj.SetLighten(1)
		Obj.SetLightenFactor(1.5)
		Judgment.Value = 1
	else
		Obj.SetLighten(0)
	end

	Obj.SetImageSkin(Judgment.Table[Judgment.Value])

	if JudgmentValue ~= 5 then
		if JudgmentValue ~= 0 then
			Obj.SetScale (Judgment.ScaleHit, Judgment.ScaleHit)
		else
			Obj.SetScale (Judgment.Scale, Judgment.Scale)
		end
	else
		Obj.SetScale (Judgment.ScaleMiss, Judgment.ScaleMiss)
	end

	Judgment.Time = 0

	Obj.SetTarget(Judgment.IndicatorObject)
	Judgment.EarlyOrLate = EarlyOrLate
end
