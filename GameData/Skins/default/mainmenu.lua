skin_require("Global/Background.lua")

Preload = {
	"MainMenu/play.png",
	"MainMenu/quit.png"
}

function InBackground(frac)
	Obj.SetAlpha( 1 - frac )
	return 1
end

function PlayBtnHover()
	Obj.SetTarget(PlayButton)
	Obj.SetImageSkin("MainMenu/playh.png")
	Obj.SetSize(256, 153)
end

function PlayBtnHoverLeave()
	Obj.SetTarget(PlayButton)
	Obj.SetImageSkin("MainMenu/play.png")
	Obj.SetSize(256, 153)
end

function ExitBtnHover()
	Obj.SetTarget(ExitButton)
	Obj.SetImageSkin("MainMenu/quith.png")
	Obj.SetSize(256, 153)
end

function ExitBtnHoverLeave()
	Obj.SetTarget(ExitButton)
	Obj.SetImageSkin("MainMenu/quit.png")
	Obj.SetSize(256, 153)
end

function BadgeZoomIn(frac)
	Obj.SetAlpha(frac)
	Obj.SetScale(frac, frac)
	return 1
end

function LogoFadeIn(frac)
	Obj.SetAlpha(frac)
	return 1
end

function BumpIn(frac)
	local S = 0.5 * (1 - frac) + 1
	Obj.SetScale (S, S)
	Obj.SetLighten (frac and 1)
	Obj.SetLightenFactor(0.5 * (1 - frac))
	return 1
end

function Init()
	BackgroundAnimation:Init()

	targBlack = Obj.CreateTarget()

	targBadge = Object2D ()
	targBadge.Image = "MainMenu/BACKs.png"
	targBadge.X = ScreenWidth / 2
	targBadge.Y = ScreenHeight / 4
	targBadge.Centered = 1
	
	targLogo = Object2D ()
	targLogo.Image = "MainMenu/FRONTs.png"
	targLogo.X = ScreenWidth / 2
	targLogo.Y = ScreenHeight / 4
	targLogo.Centered = 1
	targLogo.Alpha = 0

	Engine:AddTarget(targBadge)
	Engine:AddTarget(targLogo)
	Engine:AddAnimation(targBadge, "BadgeZoomIn", EaseOut, 0.4, 0)
	Engine:AddAnimation(targLogo, "LogoFadeIn", EaseNone, 0.4, 1.9)


	Engine:AddAnimation(targBadge, "BumpIn", EaseIn, 0.35, 2.3)
	-- Engine:AddAnimation(targLogo, "BumpIn", EaseOut, 0.15, 2.3)


	Obj.SetTarget(PlayButton)
	Obj.SetImageSkin("MainMenu/play.png")
	Obj.SetSize(256, 153)


	Obj.SetPosition (0, ScreenHeight - 153 * 2 - 40)

	Obj.SetTarget(ExitButton)
	Obj.SetImageSkin("MainMenu/quit.png")
	Obj.SetSize(256, 153)

	Obj.SetPosition (0, ScreenHeight - 153)

	Obj.SetTarget(targBlack)
	Obj.SetImageSkin("Global/filter.png")
	Obj.SetSize(ScreenWidth, ScreenHeight)
	Obj.SetAlpha(1)
	Obj.AddAnimation("InBackground", 0.35, 2.3, EaseOut)
end

function Cleanup()
	Obj.CleanTarget(targLogo)
end

badgeRotSpeed = 1080

function Update(Delta)
	BackgroundAnimation:Update(Delta)

	badgeRotSpeed = math.max(badgeRotSpeed - Delta * 240, 120)
	targBadge.Rotation = targBadge.Rotation - badgeRotSpeed * Delta
end
