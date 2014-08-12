
-- Wheel transformation
TransformX =  ScreenWidth * 13/20
CurrentTX = ScreenWidth

-- Banner transformation
BanY = 0
CurrentBanY = -192

-- DirUp Button transformation
DirUpPos = ScreenHeight - 21.5
CurrentDirUpPos = ScreenHeight + 60

-- Background
BgAlpha = 0

-- List transformation
function TransformList(Y)
  return ((Y*Y - 768*Y) / (-1055)) + CurrentTX
end

-- Screen Events
function OnSelect()

end

function OnRestore()
	CurrentTX = ScreenWidth	
	CurrentBanY = -192
	CurrentDirUpPos = ScreenHeight+60
	BgAlpha = 0
end


-- ButtonEvents
function DirUpBtnClick()
	CurrentTX = ScreenWidth
end

function KeyEvent(k, c, m)

end

function DirUpBtnHover()
	Obj.SetTarget(DirUpButton)
	Obj.SetImageSkin("up_h.png")
end

function DirUpBtnHoverLeave()
	Obj.SetTarget(DirUpButton)
	Obj.SetImageSkin("up.png")
end

function BackBtnClick()

end

function BackBtnHover()

end

function BackBtnHoverLeave()

end

function Init()
	Banner = Obj.CreateTarget()
	Obj.SetTarget(Banner)
	Obj.SetImageSkin("ssbanner.png")
	Obj.SetPosition(0, CurrentBanY)
	Obj.SetZ(18)

	Obj.SetTarget(DirUpButton)
	Obj.SetPosition(ScreenWidth/2, CurrentDirUpPos)
	Obj.SetImageSkin("up.png")
	Obj.SetCentered(1)
	Obj.SetZ(18)
end

function Cleanup()
	Obj.CleanTarget(Banner)
end

function Update(Delta)
	CurrentTX = CurrentTX + (TransformX - CurrentTX) * Delta * 8
	CurrentBanY = CurrentBanY + (BanY - CurrentBanY) * Delta * 10
	BgAlpha = BgAlpha + (1 - BgAlpha) * Delta * 5
	CurrentDirUpPos = CurrentDirUpPos + (DirUpPos - CurrentDirUpPos) * Delta * 10

	-- Transformations
	Obj.SetTarget(Banner)
	Obj.SetPosition(0, CurrentBanY)

	Obj.SetTarget(ScreenBackground)
	Obj.SetAlpha(BgAlpha)

	Obj.SetTarget(DirUpButton)
	Obj.SetPosition(ScreenWidth/2, CurrentDirUpPos)
	Obj.SetAlpha(1)
end
