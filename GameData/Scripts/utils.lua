function split(line)
	local restable = {}
	local i = 1
	for k in string.gmatch(line, "([^,]+)") do
		restable[i] = k
		i = i + 1
	end
	
	return restable
end

function AutoadjustBackground(params)
	params = params or {x = 0, y = 0, w = ScreenWidth, h = ScreenHeight}
	local x = params.x or 0
	local y = params.y or 0
	local w = params.w or ScreenWidth
	local h = params.h or ScreenHeight
	local oldWidth = Background.Width
	local oldHeight = Background.Height
	
	Background.X = x
	Background.Y = y
	
	print ("Width/Height: ", Background.Width, Background.Height)
	local HRatio = h / Background.Height
	local VRatio = w / Background.Width
	
	Background.ScaleX = math.min(HRatio, VRatio)
	Background.ScaleY = math.min(HRatio, VRatio)
	
	local modWidth = Background.ScaleX * Background.Width
	
	-- Center in box.
	if modWidth < w then
		Background.X = x + w / 2 - modWidth / 2
	end
	
	print ("Background proportions: ", Background.ScaleX, Background.ScaleY)
end