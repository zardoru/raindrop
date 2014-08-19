

Lifebar = {
	FillSize = 367,
	MarginFile = "stage-lifeb.png",
	FillFile = "stage-lifeb-s.png",
	Width = 50,
	Speed = 10
}

Judgement = {

	FadeoutTime = 2,
	FadeoutDuration = 0.5,
	Speed = 15,

	Scale = 0.20,
	ScaleHit = 0.25,
	ScaleMiss = 0.15,

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
	}

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

function Judgement.Init()
	Judgement.Object = Obj.CreateTarget()
	Obj.SetTarget(Judgement.Object)
	Obj.SetZ(24)
	Obj.SetCentered(1)
	Obj.SetScale (Judgement.Scale, Judgement.Scale)
	Obj.SetPosition (Judgement.Position.x, Judgement.Position.y)

	Judgement.Time = Judgement.FadeoutTime
end

function Judgement.Cleanup()
	Obj.CleanTarget(Judgement.Object)
end

function Judgement.Run(Delta)
	if Active ~= 0 then

		Judgement.Time = Judgement.Time + Delta

		Obj.SetTarget(Judgement.Object)
		local OldJudgeScale = Obj.GetScale()

		local DeltaScale = (Judgement.Scale - OldJudgeScale) * Delta * Judgement.Speed

		Obj.SetScale (OldJudgeScale + DeltaScale, OldJudgeScale + DeltaScale)

		if Judgement.Time > Judgement.FadeoutTime then
			local Time = Judgement.Time - Judgement.FadeoutTime
			if Time > 0 then
				local Ratio = Time / Judgement.FadeoutDuration

				if Ratio < 1 then
					Obj.SetAlpha(1 - Ratio)
				else
					Obj.SetAlpha(0)
				end
			end
		else
			Obj.SetAlpha(1)
		end
	end
end

function Judgement.Hit(JudgementValue)
	Obj.SetTarget(Judgement.Object)
	Obj.SetImageSkin(Judgement.Table[JudgementValue])

	if JudgementValue ~= 5 then
		Obj.SetScale (Judgement.ScaleHit, Judgement.ScaleHit)
	else
	    Obj.SetScale (Judgement.ScaleMiss, Judgement.ScaleMiss)
	end
	Judgement.Time = 0
end
