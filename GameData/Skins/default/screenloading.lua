IntroDuration = 0.6
ExitDuration = 0.6
Acceleration = 0

pfrac = 0

function UpdateIntro(frac)
	dFrac = frac - pfrac
	
	frac =  1 - math.pow(1 - frac, 2)
	
	Obj.SetTarget(targBadge)
	Obj.SetScale(frac, frac)
	
	Obj.SetTarget(targLogo)
	w, h = Obj.GetSize()
	Obj.SetPosition(w * frac, ScreenHeight - h)
	
	Delta = dFrac * IntroDuration
	Update(Delta)
end

function UpdateExit(frac)
	UpdateIntro(1-frac)
	Update(0.016)
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
