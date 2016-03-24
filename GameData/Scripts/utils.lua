function split(line)
	local restable = {}
	local i = 1
	for k in string.gmatch(line, "([^,]+)") do
		restable[i] = k
		i = i + 1
	end
	
	return restable
end

floor = math.floor
ceil = math.ceil
sin = math.sin
cos = math.cos
min = math.min
max = math.max

function clamp (v, mn, mx)
    return math.min(math.max(v, mn), mx)
end

function sign(x)
    if x == 0 then return 0 end
    if x > 0 then return 1 else return -1 end
end

GearWidthByChannels = {}

-- Calculate the positions of lanes based off their sizes and a start position.
function Sizeup(Pos, Size, Num, Pad)
	local GearWidthChannel = 0
	local pad = Pad or 0
	
	for i=1, Num do
		GearWidthChannel = GearWidthChannel + Size[i] + pad
	end 
	
	GearWidthByChannels[Num] = GearWidthChannel
	
	Pos[1] = Size[1] / 2 + GearStartX
	for i=2, Num do
		Pos[i] = Pos[i-1] + Size[i-1] / 2 + Size[i] / 2 + pad
	end
end

function AutoadjustBackground(params)
	params = params or {x = 0, y = 0, w = ScreenWidth, h = ScreenHeight}
	local x = params.x or 0
	local y = params.y or 0
	local w = params.w or ScreenWidth
	local h = params.h or ScreenHeight
	local oldWidth = Background.Width
	local oldHeight = Background.Height
	local Background = Background or params.Background
	
	if not Background then return end
	
	Background.X = x
	Background.Y = y

	local VRatio = h / Background.Height
	
	Background.ScaleX = VRatio
	Background.ScaleY = VRatio
	
	local modWidth = Background.ScaleX * Background.Width
	local modHeight = Background.ScaleY * Background.Height
	
	-- Center in box.
	Background.X = x + w / 2 - modWidth / 2
	Background.Y = y + h / 2 - modHeight / 2
end
