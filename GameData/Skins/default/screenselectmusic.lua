skin_require "Global/Background"
skin_require "Global/FadeInScreen"

floor = math.floor

-- Wheel item size
ItemWidth = 600
ItemHeight = 76.8

-- Wheel transformation
WheelX = 0
WheelExitX = -600
TransformX = 0
CurrentTX = -26
WheelSpeed = 900


function clamp (v, mn, mx)
    if v < mn then 
	   return mn
	else 
	   if v > mx then  
	    return mx
	   else 
	    return v
	   end
	end
end

-- List transformation
function TransformListHorizontal(Y)
  return CurrentTX
end

function sign(x)
    if x == 0 then return 0 end
    if x > 0 then return 1 else return -1 end
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
function OnItemHover(Index, Line, Selected)
	updText()
end

function OnItemHoverLeave(Index, Line, Selected)
	Wheel.CursorIndex = Wheel:GetSelectedItem()
end

function OnItemClick(Index, Line, Selected)
	Wheel:SetSelectedItem(Index)
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
	Wheel.CursorIndex = Wheel:GetSelectedItem()
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
	DirUpButton.Image = "SongSelect/up_h.png"
end

function DirUpBtnHoverLeave()
	DirUpButton.Image = "SongSelect/up.png"
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
	
	DirUpButton.Image = "SongSelect/up.png"
	DirUpButton.Centered = 1
	DirUpButton.X = ScreenWidth/2
	DirUpButton.Y = ScreenHeight - DirUpButton.Height/2
	DirUpButton.Layer = 18

	font = Fonts.TruetypeFont(GetSkinFile("font.ttf"), 24)
		
	dd = StringObject2D()
	dd.Font = font
	dd.Y = 348
	dd.X = 820
	
	WheelBackground = Object2D()
	WheelBackground.Image = "Global/white.png"
	WheelBackground.Width = ItemWidth
	WheelBackground.Height = ItemHeight

	WheelItems[Wheel:AddSprite(WheelBackground)] = function(Song, IsSelected, Index)
		if IsSelected == true and Song then
			WheelBackground.Red = 0.1
			WheelBackground.Green = 0.3
			WheelBackground.Blue = 0.7
		else
			if Index == Wheel.CursorIndex then
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
	end

	strName = StringObject2D()
	strArtist = StringObject2D()
	strDuration = StringObject2D()
	strLevel = StringObject2D()

	wheelfont = Fonts.TruetypeFont(GetSkinFile("font.ttf"), 30)
	infofont = Fonts.TruetypeFont(GetSkinFile("font.ttf"), 15)

	strName.Font = wheelfont
	strArtist.Font = font
	strDuration.Font = infofont
	strLevel.Font = infofont
	-- Transform these strings according to what they are
	WheelItemStrings[Wheel:AddString(strName)] = function(Song, IsSelected, Index, Txt)
		strName.X = strName.X + 10
		if Song then
			strName.Text = Song.Title
		else
		    strName.Text = Txt
		end
	end
	WheelItemStrings[Wheel:AddString(strArtist)] = function(Song, IsSelected, Index, Txt)
		strArtist.X = strArtist.X + 25
		strArtist.Y = strArtist.Y + 32
		if Song then
			strArtist.Text = Song.Author
			strArtist.Blue = 0.3
			strArtist.Green = 0.7
		else
			strArtist.Text = "directory..."
			strArtist.Blue = 0.3
			strArtist.Green = 0.3
		end
	end
	WheelItemStrings[Wheel:AddString(strDuration)] = function(Song, IsSelected, Index, Txt)

	end
	WheelItemStrings[Wheel:AddString(strLevel)] = function(Song, IsSelected, Index, Txt)

	end

	Engine:AddTarget(dd)
	Engine:SetUILayer(30)
end

function updText()
	local sng = Global:GetSelectedSong()
	if sng then
		local s7k = toSong7K(sng)
		if s7k then
			local diff = s7k:GetDifficulty(Global.DifficultyIndex)
			if diff then
				local author = diff.Author
				local nps = diff.Objects / diff.Duration
				if string.len(author) > 0 then
					author = " by " .. author
				end

				dd.Text = "Selected " .. diff.Name .. author .. string.format(" (%d of %d)", Global.DifficultyIndex+1, s7k.DifficultyCount) ..
					"\nChannels: " .. diff.Channels .. 
					"\nSong by " .. s7k.Author ..
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
	Wheel:SetSelectedItem(Wheel:GetSelectedItem() - yoff)
end

function Update(Delta)
	BackgroundAnimation:Update(Delta)
	CurrentTX = clamp(CurrentTX + (TransformX - CurrentTX) * Delta * 8, WheelExitX, WheelX)

	Time = Time - Delta
	

	local SelectedSongCenterY = math.floor(-Wheel:GetSelectedItem() * Wheel:GetItemHeight() + 
	ScreenHeight / 2 - Wheel:GetItemHeight()/2)
	Wheel.PendingY = SelectedSongCenterY - Wheel.ListY 
	Wheel.ScrollSpeed = -math.abs(Wheel.PendingY) / 0.25
	local deltaWheel = Wheel.ScrollSpeed * sign(Wheel.PendingY) * Delta

	if math.abs(Wheel.PendingY) < 5 then
		Wheel.ListY = Wheel.ListY + Wheel.PendingY
		Wheel.PendingY = 0
	else
		Wheel.ListY = Wheel.ListY - deltaWheel
		Wheel.PendingY = Wheel.PendingY - deltaWheel
	end

end
