
function InBackground(frac)
	Obj.SetAlpha( 1 - frac )
end

function PlayBtnHover()
	Obj.SetTarget(PlayButton)
	Obj.SetImageSkin("playh.png")
	Obj.SetSize(256, 153)
end

function PlayBtnHoverLeave()
	Obj.SetTarget(PlayButton)
	Obj.SetImageSkin("play.png")
	Obj.SetSize(256, 153)
end

function ExitBtnHover()
	Obj.SetTarget(ExitButton)
	Obj.SetImageSkin("quith.png")
	Obj.SetSize(256, 153)
end

function ExitBtnHoverLeave()
	Obj.SetTarget(ExitButton)
	Obj.SetImageSkin("quit.png")
	Obj.SetSize(256, 153)
end

function Init()

	targBadge = Obj.CreateTarget()
	targLogo = Obj.CreateTarget()
	targBlack = Obj.CreateTarget()

	Obj.SetTarget(targLogo)

	Obj.SetImageSkin("logo.png")
	Obj.SetPosition(ScreenWidth / 2, ScreenHeight / 4)
	Obj.SetCentered(1)
	Obj.SetScale(0.85, 0.85)

	Obj.SetTarget(PlayButton)
	Obj.SetSize(256, 153)

	Obj.SetPosition (0, ScreenHeight - 153 * 2 - 40)

	Obj.SetTarget(ExitButton)
	Obj.SetSize(256, 153)

	Obj.SetPosition (0, ScreenHeight - 153)

	Obj.SetTarget(targBlack)
	Obj.SetImageSkin("filter.png")
	Obj.SetAlpha(1)
	Obj.AddAnimation("InBackground", 2, 0, EaseIn)
end

function Cleanup()
	Obj.CleanTarget(targLogo)
end

function Update(Delta)
end
