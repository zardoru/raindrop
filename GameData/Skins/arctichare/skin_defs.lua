-- Scale from 3840x2160 to 1360x768

function GetScale()
	local srcres = 3840
	local dstres = 1360
	return dstres / srcres
end

SkinScale = GetScale()

-- scale values of list l by skin scale
function ScaleMap(l)
    for k,v in ipairs(l) do
        l[k] = l[k] * SkinScale
    end
end

function ScaleObj(o)
    o.ScaleX = SkinScale
    o.ScaleY = SkinScale
end

-- Player's default side on the field. 1 for left, 2 for right.
PlayerSide = 1

-- Default side for the scratch. 1 for left, 2 for right.
ScratchSide = 1
