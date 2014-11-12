FailAnimation = {
	Duration = 2
}

SuccessAnimation = {
	Duration = 5.5
}

function fcnot2f(frac)
	Obj.SetAlpha(1 - frac)
	Obj.SetScale(1 + frac * 1.5, 1 + frac * 1.5)
	Obj.SetLightenFactor(0.3 * (1 - frac))
	return 1
end

function DoFullComboAnimation()
	fcnotify = Object2D ()
	fcnotify.Image = "VSRG/fullcombo.png"

	local scalef = GearWidth / fcnotify.Width * 0.85
	fcnotify.X = ScreenWidth / 2
	fcnotify.Y = -fcnotify.Height / 2
	fcnotify.Z = 30
	fcnotify.ScaleX = scalef
	fcnotify.ScaleY = scalef
	fcnotify.Centered = 1
	fcanim = getMoveFunction(fcnotify.X, ScreenHeight + fcnotify.Height/2 * scalef, fcnotify.X, ScreenHeight*3/4)

	fcnotify2 = Object2D()
	fcnotify2.Image = "VSRG/fullcombo.png"

	fcnotify2.X = ScreenWidth / 2
	fcnotify2.Y =  ScreenHeight*3/4
	fcnotify2.ScaleX = scalef
	fcnotify2.Z = 30
	fcnotify2.ScaleY = scalef
	fcnotify2.Centered = 1
	fcnotify2.Alpha = 0
	fcnotify2.BlendMode = BlendAdd
	fcnotify2.Lighten = 1

	fcnotfade = getFadeFunction(1, 0)

	Engine:AddTarget(fcnotify)
	Engine:AddTarget(fcnotify2)
	Engine:AddAnimation(fcnotify, "fcanim", EaseOut, 0.75, 3)
	Engine:AddAnimation(fcnotify2, "fcnot2f", EaseOut, 0.25, 0.75 + 3)
	Engine:AddAnimation(fcnotify, "fcnotfade", EaseNone, 0.5, 4)
end

function FadeInBlack(frac)
	Obj.SetAlpha(frac)
	return 1
end

function ZoomVertIn(frac)
	Obj.SetAlpha(frac)
	Obj.SetScale(frac, frac)
	return 1
end

function ZoomVertOut(frac)
	Obj.SetAlpha(1 - frac)
	return 1
end

function DoSuccessAnimation()
	Black = Object2D()
	Black.Image = "Global/filter.png"
	Black.Alpha = 0
	Black.Width = ScreenWidth
	Black.Height = ScreenHeight
	Black.Z = 30

	StageClear = Object2D()
	StageClear.Image = "VSRG/stageclear.png"
	StageClear.Centered = 1
	StageClear.X = ScreenWidth / 2
	StageClear.Y = ScreenHeight / 2
	StageClear.Z = 31
	StageClear.Alpha = 0
	
	Engine:AddTarget(Black)	
	Engine:AddAnimation(Black, "FadeInBlack", EaseNone, 0.5, 3)

	if IsFullCombo then
		DoFullComboAnimation()
	end

	Engine:AddTarget(StageClear)
	Engine:AddAnimation(StageClear, "ZoomVertIn", EaseOut, 0.75, 3)
	Engine:AddAnimation(StageClear, "ZoomVertOut", EaseNone, 1, 4)
end


function FailBurst(frac)
	local TargetScaleA = 4
	local TargetScaleB = 3
	local TargetScaleC = 2
	BE.FnA.Alpha = 1 - frac
	BE.FnB.Alpha = 1 - frac
	BE.FnC.Alpha = 1 - frac

	BE.FnA.ScaleY = 1 + (TargetScaleA-1) * frac
	BE.FnB.ScaleY = 1 + (TargetScaleB-1) * frac
	BE.FnC.ScaleY = 1 + (TargetScaleC-1) * frac
	BE.FnA.ScaleX = 1 + (TargetScaleA-1) * frac
	BE.FnB.ScaleX = 1 + (TargetScaleB-1) * frac
	BE.FnC.ScaleX = 1 + (TargetScaleC-1) * frac

	return 1
end

function FailAnim(frac)

	local fnh = FailNotif.Height
	local fnw = FailNotif.Width
	local cosfacadd = 0.75
	local cos = math.cos(frac * 2 * math.pi) * cosfacadd
	local ftype = (1-frac)
	local sc = (cos + cosfacadd/2) * (ftype * ftype) * 1.2 + 1
	FailNotif.ScaleY = sc
	FailNotif.ScaleX = sc
	
	Obj.SetTarget(ScreenBackground)
	Obj.SetAlpha(1 - frac)

	return 1
end

function WhiteFailAnim(frac)
	White.Height = ScreenHeight * frac
	return 1
end

function DoFailAnimation()
	White = Object2D()
	FailNotif = Object2D()

	White.Centered = 1
	White.X = ScreenWidth / 2
	White.Y = ScreenHeight / 2
	White.Height = 0
	White.Image = "Global/white.png"
	White.Width = ScreenWidth
	FailNotif.Image = "VSRG/stagefailed.png"
	FailNotif.Centered = 1
	FailNotif.X = ScreenWidth / 2
	FailNotif.Y = ScreenHeight / 2

	White.Z = 30
	FailNotif.Z = 31

	Engine:AddTarget(White)
	Engine:AddTarget(FailNotif)

	Engine:AddAnimation(White, "WhiteFailAnim", EaseIn, 0.35, 0)
	Engine:AddAnimation(FailNotif, "FailAnim", EaseNone, 0.75, 0)
	
	BE = {}
	BE.FnA = Object2D()
	BE.FnB = Object2D()
	BE.FnC = Object2D()
	BE.FnA.Image = "VSRG/stagefailed.png"
	BE.FnB.Image = "VSRG/stagefailed.png"
	BE.FnC.Image = "VSRG/stagefailed.png"

	BE.FnA.X = ScreenWidth/2
	BE.FnB.X = ScreenWidth/2 
	BE.FnC.X = ScreenWidth/2
	BE.FnA.Y = ScreenHeight/2
	BE.FnB.Y = ScreenHeight/2
	BE.FnC.Y = ScreenHeight/2
	BE.FnA.Centered = 1
	BE.FnB.Centered = 1
	BE.FnC.Centered = 1
	BE.FnA.Alpha = 0
	BE.FnB.Alpha = 0
	BE.FnB.Alpha = 0
	BE.FnA.Z = 31
	BE.FnB.Z = 31
	BE.FnC.Z = 31

	Engine:AddTarget(BE.FnC)
	Engine:AddTarget(BE.FnB)
	Engine:AddTarget(BE.FnA)
	Engine:AddAnimation(BE.FnA, "FailBurst", EaseOut, 0.7, 0.33 * 0.75)
end
