
Acceleration = 0

function Init()
	Acceleration = 0

	targBadge = Obj.CreateTarget()
	targLogo = Obj.CreateTarget()

	Obj.SetTarget(targLogo)

	Obj.SetImageSkin("loading.png")
	w, h = Obj.GetSize()
	Obj.SetPosition(w, ScreenHeight - h)
	Obj.SetCentered(1)

	Obj.SetTarget(targBadge)
	Obj.SetImageSkin("loadingbadge.png")
	Obj.SetCentered(1)
	wb = Obj.GetSize()
	Obj.SetPosition(w - w/2 - wb/2, ScreenHeight - h)

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
