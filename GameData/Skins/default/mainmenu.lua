skin_require "Global/FadeInScreen"
game_require "AnimationFunctions"

Preload = {
	"MainMenu/play.png",
	"MainMenu/quit.png"
}

IntroDuration = 0.5
ExitDuration = 1.5
function PlayBtnHover()
	PlayButton.Texture = "MainMenu/playh.png";
	PlayButton.Width = 256
	PlayButton.Height = 153
end

function PlayBtnHoverLeave()
	PlayButton.Texture = "MainMenu/play.png";
	PlayButton.Width = 256
	PlayButton.Height = 153
end

function ExitBtnHover()
	ExitButton.Texture = "MainMenu/quith.png"
	ExitButton.Width = 256
	ExitButton.Height = 153
end

function ExitBtnHoverLeave()
	ExitButton.Texture = "MainMenu/quit.png"
	ExitButton.Width = 256
	ExitButton.Height = 153
end

function UpdateIntro(p, delta)
	local S = elastic(p)
  
  -- At 1/3rd of the screen, please.
	targBadge.Y = ScreenHeight * 3/7 * (S) - targBadge.Height
	targLogo.Y = targBadge.Y
	Update(delta)
	BGAOut(p*p)
end

function OnRunningBegin()
	ScreenFade.Out()
end

function OnRestore()
	ScreenFade.Out()
end

function OnIntroBegin()
	Engine:SetUILayer(31)
end

function OnExitBegin()
end

function UpdateExit(p, delta)
	local ease = p*p
	UpdateIntro(1-p, delta)
	FadeInA1(ease)
	BGAIn(ease)
end

function Init()
  elastic = Ease.ElasticSquare(1.5)
	ScreenFade:Init()
		
	targLogo = Engine:CreateObject() 
	targLogo.Texture = "MainMenu/FRONTs.png"
	targLogo.X = ScreenWidth / 2
	targLogo.Y = ScreenHeight / 4
	targLogo.Centered = 1
	targLogo.Alpha = 1
	targLogo.Layer = 31

	targBadge = Engine:CreateObject()
	targBadge.Texture = "MainMenu/BACKs.png"
	targBadge.X = ScreenWidth / 2
	targBadge.Y = ScreenHeight / 4
	targBadge.Centered = 1
	targBadge.Layer = 31
	
	PlayBtnHoverLeave()
	PlayButton.Y = ScreenHeight - 153 * 2 - 40
	PlayButton.Layer = 12

	ExitBtnHoverLeave()
	ExitButton.Y = ScreenHeight - 153
	ExitButton.Layer = 12
	-- Rocket UI not initialized yet...
end

function Cleanup()
end

badgeRotSpeed = 1080

function Update(Delta)

	badgeRotSpeed = math.max(badgeRotSpeed - Delta * 240, 120)
	targBadge.Rotation = targBadge.Rotation - badgeRotSpeed * Delta
	BackgroundAnimation:Update(Delta)
end
