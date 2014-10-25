function DoFullComboAnimation()
	fcnotify = Object2D ()
	fcnotify.Image = "VSRG/fullcombo.png"

	local scalef = GearWidth / fcnotify.Width * 0.85
	fcnotify.X = GearStartX + 2
	fcnotify.ScaleX = scalef
	fcnotify.Z = 30
	fcnotify.ScaleY = scalef
	fcanim = getMoveFunction(fcnotify.X, -fcnotify.Height * scalef, fcnotify.X, ScreenWidth/2 - fcnotify.Height*scalef/2)
	Engine:AddTarget(fcnotify)
	Engine:AddAnimation(fcnotify, "fcanim", EaseOut, 0.75, 0)
end