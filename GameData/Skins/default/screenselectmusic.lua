game_require "librd"
skin_require "Global/Background"
skin_require "Global/FadeInScreen"

-- Wheel item size
ItemWidth = 600
ItemHeight = 76.8

-- Wheel transformation
WheelX = 0
WheelExitX = -600
TransformX = 0
CurrentTX = -26
WheelSpeed = 900

-- List transformation
function TransformListHorizontal(Y)
  return CurrentTX
end

Wheel.ListY = 0
Wheel.PendingY = 0
Wheel.ScrollSpeed = WheelSpeed
Wheel.ItemHeight = 76.8
Wheel.ItemWidth = ItemWidth
LastSign = 0
Time = 0

function TransformPendingVertical(V)
	return 0
end

function TransformListVertical(LY)  
  	return LY
end
function OnItemHover(Index, BoundIndex, Line, Selected)
	updText()
end

function OnItemHoverLeave(Index, BoundIndex, Line, Selected)
	Wheel.CursorIndex = Wheel.SelectedIndex
end

function OnItemClick(Index, BoundIndex, Line, Song)
	print (Index, BoundIndex, Line, Song, Wheel.ListIndex, Wheel.SelectedIndex, Wheel.CursorIndex)
	if Song and (Index == Wheel.SelectedIndex) then
		Wheel:ConfirmSelection()
	else
		Wheel.SelectedIndex = Index
		if not Song then
			Wheel:ConfirmSelection() -- Go into directories inmediately
		end
	end
end


-- This gets called for every item - ideally you dispatch for every item.
WheelItems = {}
function TransformItem(Item, Song, IsSelected, Index)
	WheelItems[Item](Song, IsSelected, Index);	
end

WheelItemStrings = {}
function TransformString(Item, Song, IsSelected, Index, Txt)
	WheelItemStrings[Item](Song, IsSelected, Index, Txt);	
end

-- This recieves song and difficulty changes.
function OnSongChange(Song, Diff)
	updText()
	Wheel.CursorIndex = Wheel.SelectedIndex
end

-- Screen Events
function OnSelect()
	TransformX = WheelExitX 
	ScreenFade.In()
	return 1
end

function OnRestore()
	ScreenFade.Out()

	TransformX = WheelX
	BgAlpha = 0
	Wheel.SelectedIndex = Wheel.SelectedIndex -- force OnSongChange event
end

function OnDirectoryChange()
	TransformX = WheelX
end

-- ButtonEvents
function DirUpBtnClick()
	CurrentTX = WheelExitX
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

function gamma(o)
	o.Red = pow(o.Red, 2.2)
	o.Green = pow(o.Green, 2.2)
	o.Blue = pow(o.Blue, 2.2)
end

function Init()
	BackgroundAnimation:Init()
	ScreenFade.Init()
	
	DirUpButton.Texture = "SongSelect/up.png"
	DirUpButton.Centered = 1
	DirUpButton.X = ScreenWidth/2
	DirUpButton.Y = ScreenHeight - DirUpButton.Height/2
	DirUpButton.Layer = 18

	font = Fonts.TruetypeFont(GetSkinFile("font.ttf"), 24)
		
	dd = StringObject2D()
	dd.Font = font
	dd.Y = 327
	dd.X = 620

	Engine:AddTarget(dd)
	
	WheelBackground = Object2D()
	WheelBackground.Texture = "Global/white.png"
	WheelBackground.Width = ItemWidth
	WheelBackground.Height = ItemHeight

	WheelItems[Wheel:AddSprite(WheelBackground)] = function(Song, IsSelected, Index)
		if IsSelected == true then
			WheelBackground.Red = 0.1
			WheelBackground.Green = 0.3
			WheelBackground.Blue = 0.7
		else
			if Index == Wheel.ListIndex then
				WheelBackground.Red = 0.05
				WheelBackground.Green = 0.15
				WheelBackground.Blue = 0.35
			else
				local Nrm = Index % 2
				if Nrm == 0 then
					WheelBackground.Red = 0
					WheelBackground.Green = 0
					WheelBackground.Blue = 0
				else
					WheelBackground.Red = 0.2
					WheelBackground.Green = 0.2
					WheelBackground.Blue = 0.2
				end
			end
		end
		gamma(WheelBackground)		
	end

	strName = StringObject2D()
	strArtist = StringObject2D()
	strDuration = StringObject2D()
	strLevel = StringObject2D()
	strSubtitle = StringObject2D()

	wheelfont = Fonts.TruetypeFont(GetSkinFile("font.ttf"), 30)
	infofont = Fonts.TruetypeFont(GetSkinFile("font.ttf"), 18)

	strName.Font = wheelfont
	strArtist.Font = font
	strDuration.Font = infofont
	strLevel.Font = infofont
	strSubtitle.Font = infofont 

	local dur_x = 50
	
	-- Transform these strings according to what they are
	WheelItemStrings[Wheel:AddString(strName)] = function(Song, IsSelected, Index, Txt)
		strName.X = strName.X + 10
		if Song then
			strName.Text = Song.Title
		else
		    strName.Text = Txt
		end

		local w = wheelfont:GetLength(strName.Text)
		local m = ItemWidth - dur_x - 20
		if w > m then
			strName.ScaleX = m / w
		else
			strName.ScaleX = 1
		end

	end
	WheelItemStrings[Wheel:AddString(strArtist)] = function(Song, IsSelected, Index, Txt)
		strArtist.X = strArtist.X + 10
		strArtist.Y = strArtist.Y + 45
		strArtist.Red = 1
		if Song then
			strArtist.Text = "by " .. Song.Author
			strArtist.Blue = 0.3
			strArtist.Green = 0.7
		else
			strArtist.Text = "directory..."
			strArtist.Blue = 0.3
			strArtist.Green = 0.3
		end

		local w = infofont:GetLength(strArtist.Text)
		local m = ItemWidth - 20

		if w > m then
			strArtist.ScaleX = m / w
		else
			strArtist.ScaleX = 1
		end

		gamma(strArtist)
	end
	WheelItemStrings[Wheel:AddString(strDuration)] = function(Song, IsSelected, Index, Txt)
		if Song then
			local sng = toSong7K(Song)
			if not sng then
				sng = toSongDC(Song)
			end

			local s = floor(sng:GetDifficulty(0).Duration % 60)
			local m = floor( (sng:GetDifficulty(0).Duration - s) / 60 )
			strDuration.Text = string.format("%d:%02d", m, s)
		else
			strDuration.Text = ""
		end
		strDuration.X = strDuration.X + ItemWidth - dur_x
		strDuration.Y = strDuration.Y + 10
	end
	WheelItemStrings[Wheel:AddString(strLevel)] = function(Song, IsSelected, Index, Txt)
		
	end

	WheelItemStrings[Wheel:AddString(strSubtitle)] = function(Song, IsSelected, Index, Txt)
		strSubtitle.X = strSubtitle.X + 10
		strSubtitle.Y = strSubtitle.Y + 30
		if Song then
			strSubtitle.Text = Song.Subtitle
		else
			strSubtitle.Text = ""
		end

		local w = infofont:GetLength(strSubtitle.Text)
		local m = ItemWidth - 20

		if w > m then
			strSubtitle.ScaleX = m / w
		else
			strSubtitle.ScaleX = 1
		end
	end


	WheelSeparator = Engine:CreateObject()
	WheelSeparator.Texture = "Global/white.png"
	WheelSeparator.Height = ScreenHeight
	WheelSeparator.Width = 5
	WheelSeparator.Y = 0
	
	wheeltick = Engine:CreateObject()
	wheeltick.Texture = "Global/white.png"
	wheeltick.Height = 8
	wheeltick.Width = 16
	wheeltick.Layer = 25
	
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
	Wheel.SelectedIndex = Wheel.SelectedIndex - yoff
	Wheel.CursorIndex = Wheel.SelectedIndex
end

function Update(Delta)
	BackgroundAnimation:Update(Delta)
	CurrentTX = clamp(CurrentTX + (TransformX - CurrentTX) * Delta * 8, WheelExitX, WheelX)

	WheelSeparator.X = CurrentTX + ItemWidth
	wheeltick.Width = math.max(16, ScreenWidth / Wheel.ItemCount)
	wheeltick.X = (Wheel.SelectedIndex % Wheel.ItemCount) / (Wheel.ItemCount - 1) * (ScreenWidth - wheeltick.Width)
	wheeltick.Y = 86
	
	Time = Time - Delta
	

	local SelectedSongCenterY = math.floor(-Wheel.SelectedIndex * Wheel.ItemHeight + 
	ScreenHeight / 2 - Wheel.ItemHeight/2)
	Wheel.PendingY = SelectedSongCenterY - Wheel.ListY 
	Wheel.ScrollSpeed = -math.abs(Wheel.PendingY) / 0.25

	-- don't overshoot
	local deltaWheel = sign(Wheel.PendingY) * math.min(Wheel.ScrollSpeed * Delta, math.abs(SelectedSongCenterY - Wheel.ListY))

	if math.abs(Wheel.PendingY) < 1 then
		Wheel.ListY = Wheel.ListY + Wheel.PendingY
		Wheel.PendingY = 0
	else
		Wheel.ListY = Wheel.ListY - deltaWheel
		Wheel.PendingY = Wheel.PendingY - deltaWheel
	end

end
