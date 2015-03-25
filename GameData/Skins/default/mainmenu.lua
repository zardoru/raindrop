skin_require("Global/Background.lua")

Preload = {
	"MainMenu/play.png",
	"MainMenu/quit.png"
}

function InBackground(frac)
	targBlack.Alpha = 1 - frac
	return 1
end

function PlayBtnHover()
	PlayButton.Image = "MainMenu/playh.png";
	PlayButton.Width = 256
	PlayButton.Height = 153
end

function PlayBtnHoverLeave()
	PlayButton.Image = "MainMenu/play.png";
	PlayButton.Width = 256
	PlayButton.Height = 153
end

function ExitBtnHover()
	ExitButton.Image = "MainMenu/quith.png"
	ExitButton.Width = 256
	ExitButton.Height = 153
end

function ExitBtnHoverLeave()
	ExitButton.Image = "MainMenu/quit.png"
	ExitButton.Width = 256
	ExitButton.Height = 153
end

function BadgeZoomIn(frac)
	targBadge.Alpha = frac
	targBadge:SetScale(frac)
	return 1
end

function LogoFadeIn(frac)
	targLogo.Alpha = frac
	return 1
end

function BumpIn(frac)
	local S = 0.5 * (1 - frac) + 1
	targBadge:SetScale (S)
	targBadge.Lighten = (frac and 1)
	targBadge.LightenFactor = 0.5 * (1 - frac)
	return 1
end

function Init()
	BackgroundAnimation:Init()

	targBlack = Engine:CreateObject()

	targBadge = Engine:CreateObject()
	targBadge.Image = "MainMenu/BACKs.png"
	targBadge.X = ScreenWidth / 2
	targBadge.Y = ScreenHeight / 4
	targBadge.Centered = 1
	
	targLogo = Engine:CreateObject() 
	targLogo.Image = "MainMenu/FRONTs.png"
	targLogo.X = ScreenWidth / 2
	targLogo.Y = ScreenHeight / 4
	targLogo.Centered = 1
	targLogo.Alpha = 0

	Engine:AddAnimation(targBadge, "BadgeZoomIn", EaseOut, 0.4, 0)
	Engine:AddAnimation(targBadge, "BumpIn", EaseIn, 0.35, 2.3)
	Engine:AddAnimation(targLogo, "LogoFadeIn", EaseNone, 0.4, 1.9)

	PlayBtnHoverLeave()
	PlayButton.Y = ScreenHeight - 153 * 2 - 40
	PlayButton.Layer = 18

	ExitBtnHoverLeave()
	ExitButton.Y = ScreenHeight - 153
	ExitButton.Layer = 18
	
	targBlack.Image = "Global/filter.png"
	targBlack.Width = ScreenWidth
	targBlack.Height = ScreenHeight
	targBlack.Alpha = 1
	Engine:AddAnimation(targBlack, "InBackground", EaseOut, 0.35, 2.3)
end

function Cleanup()
end

badgeRotSpeed = 1080

function Update(Delta)
	BackgroundAnimation:Update(Delta)

	badgeRotSpeed = math.max(badgeRotSpeed - Delta * 240, 120)
	targBadge.Rotation = targBadge.Rotation - badgeRotSpeed * Delta
end
