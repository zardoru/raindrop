function getMoveFunction(sX, sY, eX, eY, obj)
	return function(frac)
		obj.X = sX + (eX - sX)*frac 
		obj.Y = sY + (eY - sY)*frac
		return 1
	end
end

function getUncropFunction(w, h, iw, ih, obj)
	return function(frac)
		obj.Width = w 
		obj.Height = h*(1-frac)
		obj:SetCropByPixels(0, iw, ih*frac*0.5, (ih - (ih*frac*0.5)))
		return 1
	end
end

function getFadeFunction(sF, eF, obj)
	return function (frac)
		obj.Alpha = sF + (eF - sF)*frac
		return 1
	end
end

-- bezier over 1 dimension
local function Bez1D (p2, p1, t)
	local cx = 3 * p1
	local bx = 3 * (p2 - p1) - cx
	local ax = 1 - cx - bx
	local xt = ax * t*t*t + bx * t*t + cx * t

	return xt
end

-- 1D cubic bez derivative
local function BezdAdT(aa1, aa2, t)
	return 3 * (1 - 3 * aa2 + 3 * aa1) * t * t + 
				 2 * (3 * aa2 - 6 * aa1) * t +
				 3 * aa1
end

-- p[1] == x, p[2] == y
function CubicBezier(p1, p2, t)
	return Bez1D(p2[1], p1[1], t), Bez1D(p2[2], p1[2], t)
end

Ease = {
  ElasticSquare = function(p)
    local attn = 1 + 1 - math.asin(1.0 / p) * 2.0 / math.pi
		local pi = math.pi
		local sin = math.sin
    return function (x)
      return sin(x*x * pi / 2.0 * attn) * p
    end
  end,
	CubicBezier = function (p1, p2)
		--[[ 
			based off
		  http://greweb.me/2012/02/bezier-curve-based-easing-functions-from-concept-to-implementation/
		]]

		return function(t)

			-- newton raphson
			local function tforx(ax)
				local guess = ax
				for i=1,4 do 
					local cs = BezdAdT(p1[1], p2[1], guess)
					if cs == 0 then return guess end 
					
					local cx = Bez1D(p1[1], p2[1], guess) - ax
					guess = guess - cx / cs 
				end

				return guess
			end

			return Bez1D(p1[2], p2[2], tforx(t))
		end
	end
}
