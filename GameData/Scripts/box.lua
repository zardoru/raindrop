Box = {
	TopLeft = 1,
	Top = 2,
	TopRight = 3,
	Left = 4,
	Center = 5,
	Right = 6,
	BottomLeft = 7,
	Bottom = 8,
	BottomRight = 9,


	On = nil,
	Off = 0,
}

Box.__index = Box

--[[
	If the index exists, use the specified corner if it's not zero, otherwise
	use the default corner, and show it.
	If the 'Transform' parameter exists. append to all objects this transformation.

	params.Width =  The width of the interior of the box. Must be larger than the sum of the corners' width.
	params.Height = Height of the interior of the box. Must be larger than corners.
	params.Source = (optional) The image source. If non-existant, use Global/box.png. This picture will be divided into a 3x3 grid.
	params[1..9] = Replace, on, or off sides. If on, is nil, if off, is 0, if we should replace, the index will be used according to the definitions in the Box table.
	params.Layer = Set all objects to this layer, or 16 by default.
	params.StretchCenter = if true (default) stretch the center all across the area of the box.
]]

function Box:ModifyTextureBounds(index, Object)
	if index == Box.TopLeft then
		Object:SetCropByPixels(0, self.CellSize.Width, 0, self.CellSize.Height)
	elseif index == Box.Top then
		Object:SetCropByPixels(self.CellSize.Width, self.CellSize.Width * 2, 0, self.CellSize.Height)
	elseif index == Box.TopRight then
		Object:SetCropByPixels(self.CellSize.Width * 2, self.CellSize.Width * 3, 0, self.CellSize.Height)
	elseif index == Box.Left then
		Object:SetCropByPixels(0, self.CellSize.Width, self.CellSize.Height, self.CellSize.Height * 2)
	elseif index == Box.Center then
		Object:SetCropByPixels(self.CellSize.Width, self.CellSize.Width * 2, self.CellSize.Height, self.CellSize.Height * 2)
	elseif index == Box.Right then
		Object:SetCropByPixels(self.CellSize.Width * 2, self.CellSize.Width * 3, self.CellSize.Height, self.CellSize.Height * 2)
	elseif index == Box.BottomLeft then
		Object:SetCropByPixels(0, self.CellSize.Width, self.CellSize.Height * 2, self.CellSize.Height * 3)
	elseif index == Box.Bottom then
		Object:SetCropByPixels(self.CellSize.Width, self.CellSize.Width * 2, self.CellSize.Height * 2, self.CellSize.Height * 3)
	elseif index == Box.BottomRight then
		Object:SetCropByPixels(self.CellSize.Width * 2, self.CellSize.Width * 3, self.CellSize.Height * 2, self.CellSize.Height * 3)
	end
end

function Box:CreateCorner(index, replace)

	if replace == nil then
		replace = index
	end

	if replace == 0 then -- if it's nil, on the other hand..
		return
	end

	local BoxStartInteriorX = self.CellSize.Width
	local BoxStartInteriorY = self.CellSize.Height
	local BoxBottom = BoxStartInteriorY + self.InteriorSize.Height
	local BoxRight = BoxStartInteriorX + self.InteriorSize.Width

	self.Objects = self.Objects or {}
	self.Objects[#self.Objects+1] = Engine:CreateObject()
	local Object = self.Objects[#self.Objects]
	Object.Image = self.Image
	Object.Width = self.CellSize.Width
	Object.Height = self.CellSize.Height
	Object.Layer = self.Layer

	if self.Transform then
		Object.ChainTransformation = self.Transform
	end

	if index == Box.TopLeft then
		Object.X = 0
		Object.Y = 0
	elseif index == Box.Top then
		Object.X = BoxStartInteriorX
		Object.Y = 0
		Object.Width = self.InteriorSize.Width
	elseif index == Box.TopRight then
		Object.X = BoxRight
		Object.Y = 0
	elseif index == Box.Left then
		Object.X = 0
		Object.Y = BoxStartInteriorY
		Object.Height = self.InteriorSize.Height
	elseif index == Box.Center then
		if self.StretchCenter then
			Object.X = 0
			Object.Y = 0
			Object.Width = self.InteriorSize.Width + self.CellSize.Width * 2
			Object.Height = self.InteriorSize.Height + self.CellSize.Height * 2
			Object.Layer = self.Layer-1
		else
			Object.X = BoxStartInteriorX
			Object.Y = BoxStartInteriorY
			Object.Width = self.InteriorSize.Width
			Object.Height = self.InteriorSize.Height
		end
	elseif index == Box.Right then
		Object.X = BoxRight
		Object.Y = BoxStartInteriorY
		Object.Height = self.InteriorSize.Height
	elseif index == Box.BottomLeft then
		Object.X = 0
		Object.Y = BoxBottom
	elseif index == Box.Bottom then
		Object.X = BoxStartInteriorX
		Object.Y = BoxBottom
		Object.Width = self.InteriorSize.Width
	elseif index == Box.BottomRight then
		Object.X = BoxRight
		Object.Y = BoxBottom
	end

	self:ModifyTextureBounds(replace, Object)
end

function Box:CreateCorners(params)
	self.Image = params.Source or "Global/box.png"
	self.Transform = params.Transform
	self.Layer = params.Layer or 20

	self.Master = Engine:CreateObject()
	self.Master.Image = self.Image
	self.Master.Alpha = 0

	self.StretchCenter = params.StretchCenter or true
	self.CellSize = { Width = self.Master.Width/3, Height = self.Master.Height/3 }
	self.InteriorSize = { Width = params.Width or self.CellSize.Width, Height = params.Height or self.CellSize.Height }

	if self.Transform == nil then
		print "no transform.."
	end

	for i=1, 9 do
		self:CreateCorner(i, params[i])
	end
end

function Box:Create(params)
	if type(params) ~= "table" then
		return nil
	end

	local out = {}
	setmetatable(out, self)
	out:CreateCorners(params)

	return out
end

return Box
