skin_require("Loading/phrases.lua")

IntroDuration = 0.35
ExitDuration = 0.35
Acceleration = 0

pfrac = 0

function UpdateIntro(frac)
	dFrac = frac - pfrac
	pfrac = frac
	
	targBadge:SetScale(frac)
	
	frac =  1 - math.pow(1 - frac, 2)
	

	targLogo.X = targLogo.Width * frac
	targLogo.Y = ScreenHeight - targLogo.Height
	
	BG.Alpha = frac
	
	Phrases.Fade(frac)
	Delta = dFrac * IntroDuration
	Update(Delta)
end

function UpdateExit(frac)
	UpdateIntro(1-frac)
end

function Init()
	Acceleration = 0
	
	targLogo =  Engine:CreateObject()
	targLogo.Image = "Loading/loading.png"
	w = targLogo.Width
	h = targLogo.Height
	targLogo.Centered = 1
	targLogo.Layer = 16

	targBadge = Engine:CreateObject()
	
	targBadge.Image = "Loading/loadingbadge.png"
	targBadge.Centered = 1
	targBadge.Width = 64
	targBadge.Height = 64
	wb = targBadge.Width
	targBadge.X = (w - w/2 - wb/2)
	targBadge.Y = ScreenHeight - h
	targBadge.Layer = 16
	
	BG = Engine:CreateObject()
	BG.Image = "STAGEFILE" -- special constant
	BG.Centered = 1
	BG.X = ScreenWidth / 2
	BG.Y = ScreenHeight / 2
	
	local HRatio = ScreenHeight / BG.Height
	local VRatio = ScreenWidth / BG.Width
	
	BG.ScaleX = math.min(HRatio, VRatio)
	BG.ScaleY = math.min(HRatio, VRatio)
	BG.Layer = 10
	BG.Alpha = 0
	
	Phrases.Init()
end

function Cleanup()
end

function Update(Delta)
	Acceleration = Acceleration + Delta

	local Bump = Acceleration - math.floor(Acceleration)
	targLogo:SetScale(1.1 - 0.1 * Bump)

	targBadge.Rotation = targBadge.Rotation + (6)
end
