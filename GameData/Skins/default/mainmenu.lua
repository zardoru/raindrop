skin_require("Global/FadeInScreen.lua")

Preload = {
	"MainMenu/play.png",
	"MainMenu/quit.png"
}

IntroDuration = 0.25
ExitDuration = 0.5
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

function UpdateIntro(p, delta)
	local S = math.sin(-5 * math.pi * (p + 1)) * math.pow(2, -10 * p) + 1
	targBadge.Y = ScreenHeight/2*(S) - targBadge.Height
	targLogo.Y = targBadge.Y
	Update(delta)
end

function OnRunningBegin()
	ScreenFade.Out()
end

function OnIntroBegin()
end

function OnExitBegin()
	ScreenFade.In()
end

function UpdateExit(p, delta)
	local ease = p*p
	UpdateIntro(1-p)
	print (delta)
	Update(delta)
end

function Init()
	ScreenFade:Init()
	
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
	targLogo.Alpha = 1

	PlayBtnHoverLeave()
	PlayButton.Y = ScreenHeight - 153 * 2 - 40
	PlayButton.Layer = 18

	ExitBtnHoverLeave()
	ExitButton.Y = ScreenHeight - 153
	ExitButton.Layer = 18
end

function Cleanup()
end

badgeRotSpeed = 1080

function Update(Delta)

	badgeRotSpeed = math.max(badgeRotSpeed - Delta * 240, 120)
	targBadge.Rotation = targBadge.Rotation - badgeRotSpeed * Delta
end
