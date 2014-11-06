skin_require("Global/Background.lua")

-- Wheel transformation
TransformX =  ScreenWidth * 15/20
CurrentTX = ScreenWidth

-- List transformation
function TransformList(Y)
  return ((Y*Y - 768*Y) / (1055)) + CurrentTX
end

function InBanner(frac)
	Obj.SetPosition( 0, -192 * (1-frac) )
	return 1
end

function InUpBtn(frac)
	Obj.SetPosition( ScreenWidth / 2, ScreenHeight + 60 - 81.5 * frac )
	return 1
end

function InBackground(frac)
	Obj.SetAlpha(frac)
	return 1
end

-- Screen Events
function OnSelect()
	TransformX = ScreenWidth
	return 2
end

function OnRestore()
	TransformX =  ScreenWidth * 15/20
	CurrentTX = ScreenWidth	
	BgAlpha = 0
	Obj.SetTarget(DirUpButton)
	Obj.AddAnimation( "InUpBtn", 0.5, 0, EaseOut )

	Obj.SetTarget(Banner)
	Obj.AddAnimation( "InBanner", 0.5, 0, EaseOut )

	Obj.SetTarget(ScreenBackground)
	Obj.AddAnimation( "InBackground", 0.5, 0, EaseOut )
end


-- ButtonEvents
function DirUpBtnClick()
	CurrentTX = ScreenWidth
end

function KeyEvent(k, c, m)

end

function DirUpBtnHover()
	Obj.SetTarget(DirUpButton)
	Obj.SetImageSkin("SongSelect/up_h.png")
end

function DirUpBtnHoverLeave()
	Obj.SetTarget(DirUpButton)
	Obj.SetImageSkin("SongSelect/up.png")
end

function BackBtnClick()

end

function BackBtnHover()

end

function BackBtnHoverLeave()

end

function Init()

	BackgroundAnimation:Init()

	Banner = Obj.CreateTarget()
	Obj.SetTarget(Banner)
	Obj.SetImageSkin("SongSelect/ssbanner.png")
	Obj.SetZ(18)

	Obj.SetTarget(DirUpButton)
	Obj.SetImageSkin("SongSelect/up.png")
	Obj.SetCentered(1)
	Obj.SetZ(18)

	font = Fonts.TruetypeFont(Obj.GetSkinDirectory() .. "font.ttf", 24)
	
	dd = StringObject2D()
	dd.Font = font
	dd.Y = 348
	dd.X = 120
	Engine:AddTarget(dd)
end

function Cleanup()
	Obj.CleanTarget(Banner)
end

function Update(Delta)
	BackgroundAnimation:Update(Delta)
	CurrentTX = CurrentTX + (TransformX - CurrentTX) * Delta * 8

	local sng = Global:GetSelectedSong()
	if sng then
		local s7k = toSong7K(sng)
		if s7k then
			local diff = s7k:GetDifficulty(Global.DifficultyIndex)
			if diff then
				dd.Text = "Difficulty: " .. diff.Name .. " by " .. diff.Author ..
					"\nChannels: " .. diff.Channels .. 
					"\nSong by " .. s7k.Author

			end
		end
	else
		dd.Text = ""
	end
end
