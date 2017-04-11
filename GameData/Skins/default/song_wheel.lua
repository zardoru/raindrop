-- Read "state" as the wheel's state
local State = {
    X = 0,
    ListY = 0,
    PendingY = 0,
    LastSign = 0,
    Time = 0,
    ScrollSpeed = 0,
}


local WheelItems = {}

-- Wheel item size
local ItemWidth = 600
local ItemHeight = 76.8

-- Wheel transformation
local WheelEnterX = 0
local WheelExitX = -ItemWidth
local WheelSpeed = 900

local function FracToWheelIndex(t)
    return t * Wheel.DisplayItemCount + Wheel.DisplayStartIndex
end

-- List transformation
function TransformListHorizontal(t)
  return State.X
end

function TransformListVertical(t)  
	  return State.ListY + FracToWheelIndex(t) * ItemHeight 
end

function TransformListWidth(t)
	return ItemWidth
end

function TransformListHeight(t)
	return ItemHeight
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
function TransformItem(Item, Song, IsSelected, Index)
	WheelItems[Item](Song, IsSelected, Index);	
end

local WheelItemStrings = {}
function TransformString(Item, Song, IsSelected, Index, Txt)
	WheelItemStrings[Item](Song, IsSelected, Index, Txt);	
end

-- This recieves song and difficulty changes.
function OnSongChange(Song, Diff)
	updText()
	Wheel.CursorIndex = Wheel.SelectedIndex
end

function CreateWheelItems()
    local WheelBackground = Object2D()
    State.WheelBackground = WheelBackground
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
		--(WheelBackground)		
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

	Wheel.DisplayItemCount = ceil(ScreenHeight / ItemHeight) + 1
	--Wheel.DisplayItemOffset = - Wheel.DisplayItemCount / 2
end

function UpdateWheel(Delta)
    State.X = clamp(State.X + (WheelEnterX - State.X) * Delta * WheelSpeed, WheelExitX, WheelEnterX)

	WheelSeparator.X = State.X + ItemWidth
	wheeltick.Width = math.max(16, ScreenWidth / Wheel.ItemCount)
	wheeltick.X = (Wheel.SelectedIndex % Wheel.ItemCount) / (Wheel.ItemCount - 1) * (ScreenWidth - wheeltick.Width)
	wheeltick.Y = 86
	
    local Offset = ScreenHeight / 2 - ItemHeight / 2
	local SelectedSongCenterY = math.floor(-Wheel.SelectedIndex * ItemHeight + Offset)
	State.PendingY = SelectedSongCenterY - State.ListY 
	State.ScrollSpeed = -math.abs(State.PendingY) / 0.25

	-- don't overshoot
    local dist = math.abs(SelectedSongCenterY - State.ListY)
	local deltaWheel = sign(State.PendingY) * math.min(State.ScrollSpeed * Delta, dist)

	if math.abs(State.PendingY) < 1 then
		State.ListY = State.ListY + State.PendingY
		State.PendingY = 0
	else
		State.ListY = State.ListY - deltaWheel
		State.PendingY = State.PendingY - deltaWheel
	end

	-- Display -halfCount to +halfCount 
	-- what's the center's current index?
	local centerCurrentIndex = floor(-State.ListY / ItemHeight)
	Wheel.DisplayStartIndex = centerCurrentIndex

end