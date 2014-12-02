IntroDuration = 0.35
ExitDuration = 0.35
Acceleration = 0

pfrac = 0

function UpdateIntro(frac)
	dFrac = frac - pfrac
	pfrac = frac
	
	frac =  1 - math.pow(1 - frac, 2)
	
	Obj.SetTarget(targBadge)
	Obj.SetScale(frac, frac)
	
	Obj.SetTarget(targLogo)
	w, h = Obj.GetSize()
	Obj.SetPosition(w * frac, ScreenHeight - h)
	
	local alpha = frac * 4
	if alpha > 1 then alpha = 1 end
	
	BG.Alpha = alpha
	
	Delta = dFrac * IntroDuration
	Update(Delta)
end

function UpdateExit(frac)
	dFrac = frac - pfrac
	pfrac = frac
	
	UpdateIntro(1-frac)
	
	local alpha = frac - 0.75
	if alpha < 0 then alpha = 0 else alpha = 4 * alpha end
	
	BG.Alpha = alpha
	Delta = dFrac * ExitDuration
	Update(Delta)
end

function Init()
	Acceleration = 0

	targBadge = Obj.CreateTarget()
	targLogo = Obj.CreateTarget()

	Obj.SetTarget(targLogo)

	Obj.SetImageSkin("Loading/loading.png")
	w, h = Obj.GetSize()
	Obj.SetCentered(1)

	Obj.SetTarget(targBadge)
	Obj.SetImageSkin("Loading/loadingbadge.png")
	Obj.SetCentered(1)
	wb = Obj.GetSize()
	Obj.SetPosition((w - w/2 - wb/2), ScreenHeight - h)
	
	BG = Object2D()
	BG.Image = "STAGEFILE" -- special constant
	BG.Centered = 1
	BG.X = ScreenWidth / 2
	BG.Y = ScreenHeight / 2
	BG.Alpha = 0
	
	Engine:AddTarget(BG)
end

function Cleanup()
	Obj.CleanTarget(targLogo)
	Obj.CleanTarget(targBadge)
end

function Update(Delta)
	Acceleration = Acceleration + Delta
	Obj.SetTarget(targLogo)

	local Bump = Acceleration - math.floor(Acceleration)
	Obj.SetScale(1.1 - 0.1 * Bump , 1.1 - 0.1 * Bump)

	Obj.SetTarget(targBadge)
	Obj.Rotate(6)
end
