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
	AdjustInBox(Background, params)
end
