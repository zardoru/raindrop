game_require "librd"
skin_require "Global/Background"
skin_require "Global/FadeInScreen"

skin_require "song_wheel.lua"


-- Screen Events
function OnSelect()
	TransformX = WheelExitX 
	ScreenFade.In()
	return 1
end

function OnRestore()
	ScreenFade.Out()

	Wheel.SelectedIndex = Wheel.SelectedIndex -- force OnSongChange event
end

function OnDirectoryChange()
	TransformX = WheelX
end

-- ButtonEvents
function DirUpBtnClick()
end

function KeyEvent(k, c, m)

end

function DirUpBtnHover()
	DirUpButton.Texture = "SongSelect/up_h.png"
end

function DirUpBtnHoverLeave()
	DirUpButton.Texture = "SongSelect/up.png"
end

function BackBtnClick()

end

function BackBtnHover()

end

function BackBtnHoverLeave()

end

function Init()
	BackgroundAnimation:Init()
	ScreenFade.Init()

	font = Fonts.TruetypeFont(GetSkinFile("font.ttf"), 24)
		
	dd = StringObject2D()
	dd.Font = font
	dd.Y = 327
	dd.X = 620

	Engine:AddTarget(dd)
	
	CreateWheelItems()
	
	Engine:SetUILayer(24)
end

function updText()
	local sng = Global:GetSelectedSong()
	if sng then
		local s7k = toSong7K(sng)
		if s7k then
			local diff = Global:GetDifficulty(0)
			if diff then
				local author = diff.Author
				local nps = diff.Objects / diff.Duration
				if string.len(author) > 0 then
					author = " by " .. author
				end

				dd.Text = "Selected " .. diff.Name .. author .. 
					string.format("\n%d of %d", Wheel.DifficultyIndex+1, s7k.DifficultyCount) ..
					"\n" .. diff.Channels .. " Channels" ..
					"\nLevel " .. diff.Level .. 
					" (" .. string.format("%.02f", nps) .. " nps)"

			end
		else
			dd.Text = "dotcur mode song";
		end
	else
		dd.Text = ""
	end
end

function Cleanup()
end

function ScrollEvent(xoff, yoff)
	print (yoff, Wheel.SelectedIndex, Wheel.SelectedIndex - yoff)
	Wheel.SelectedIndex = Wheel.SelectedIndex - yoff
	Wheel.CursorIndex = Wheel.SelectedIndex
end

function Update(Delta)
	BackgroundAnimation:Update(Delta)
	UpdateWheel(Delta)
end
