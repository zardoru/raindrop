
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