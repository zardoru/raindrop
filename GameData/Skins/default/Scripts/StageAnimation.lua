FailAnimation = {
	Duration = 2
}

SuccessAnimation = {
	Duration = 5.5
}

function fcnot2f(frac)
	fcnotify2.Alpha = (1 - frac)
	fcnotify2:SetScale(1 + frac * 1.5)
	fcnotify2.LightenFactor = (0.3 * (1 - frac))
	return 1
end

function DoFullComboAnimation()
	fcnotify = Engine:CreateObject ()
	fcnotify.Texture = "VSRG/fullcombo.png"

	fcnotify.X = ScreenWidth / 2
	fcnotify.Y = -fcnotify.Height / 2
	fcnotify.Z = 30
	fcnotify.Centered = 1
	fcanim = getMoveFunction(fcnotify.X, ScreenHeight + fcnotify.Height/2, fcnotify.X, ScreenHeight*3/4, fcnotify)

	fcnotify2 = Engine:CreateObject()
	fcnotify2.Texture = "VSRG/fullcombo.png"

	fcnotify2.X = ScreenWidth / 2
	fcnotify2.Y =  ScreenHeight*3/4
	fcnotify2.Z = 30
	fcnotify2.Centered = 1
	fcnotify2.Alpha = 0
	fcnotify2.BlendMode = BlendAdd
	fcnotify2.Lighten = 1

	fcnotfade = getFadeFunction(1, 0, fcnotify)

	Engine:AddAnimation(fcnotify, "fcanim", EaseOut, 0.75, 3)
	Engine:AddAnimation(fcnotify2, "fcnot2f", EaseOut, 0.25, 0.75 + 3)
	Engine:AddAnimation(fcnotify, "fcnotfade", EaseNone, 0.5, 4)
end

function FadeInBlack(frac)
	Black.Alpha = (frac)
	return 1
end

function ZoomVertIn(frac)
	StageClear.Alpha = (frac)
	StageClear:SetScale(frac)
	return 1
end

function ZoomVertOut(frac)
	StageClear.Alpha = (1 - frac)
	return 1
end

function FadeToBlack()
	Black = Engine:CreateObject()
	Black.Texture = "Global/filter.png"
	Black.Alpha = 0
	Black.Width = ScreenWidth
	Black.Height = ScreenHeight
	Black.Z = 29
	Engine:AddAnimation(Black, "FadeInBlack", EaseNone, 0.5, 3)
end

function DoSuccessAnimation()
	FadeToBlack()

	StageClear = Engine:CreateObject()
	StageClear.Texture = "VSRG/stageclear.png"
	StageClear.Centered = 1
	StageClear.X = ScreenWidth / 2
	StageClear.Y = ScreenHeight / 2
	StageClear.Z = 31
	StageClear.Alpha = 0
	

	if IsFullCombo then
		DoFullComboAnimation()
	end

	Engine:Sort()
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
	
	return 1
end

function WhiteFailAnim(frac)
	White.Height = ScreenHeight * frac
	return 1
end

function DoFailAnimation()
	White = Engine:CreateObject()
	FailNotif = Engine:CreateObject()

	White.Centered = 1
	White.X = ScreenWidth / 2
	White.Y = ScreenHeight / 2
	White.Height = 0
	White.Texture = "Global/white.png"
	White.Width = ScreenWidth
	FailNotif.Texture = "VSRG/stagefailed.png"
	FailNotif.Centered = 1
	FailNotif.X = ScreenWidth / 2
	FailNotif.Y = ScreenHeight / 2

	White.Z = 30
	FailNotif.Z = 31

	Engine:AddAnimation(White, "WhiteFailAnim", EaseIn, 0.35, 0)
	Engine:AddAnimation(FailNotif, "FailAnim", EaseNone, 0.75, 0)
	
	BE = {}
	BE.FnA = Engine:CreateObject()
	BE.FnB = Engine:CreateObject()
	BE.FnC = Engine:CreateObject()
	BE.FnA.Texture = "VSRG/stagefailed.png"
	BE.FnB.Texture = "VSRG/stagefailed.png"
	BE.FnC.Texture = "VSRG/stagefailed.png"

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

	Engine:Sort()
	Engine:AddAnimation(BE.FnA, "FailBurst", EaseOut, 0.7, 0.33 * 0.75)
end
