function split(line, _sep)
    local sep = _sep or ","
	local restable = {}
	local i = 1
	for k in string.gmatch(line, "([^" .. sep .. "]+)") do
		restable[i] = k
		i = i + 1
	end

	return restable
end

string.split = split

floor = math.floor
ceil = math.ceil
sin = math.sin
cos = math.cos
min = math.min
max = math.max
pow = math.pow

function clamp (v, mn, mx)
    return math.min(math.max(v, mn), mx)
end

function sign(x)
    if x == 0 then return 0 end
    if x > 0 then return 1 else return -1 end
end

function sum(l)
	local rt = 0
	for k,v in ipairs(l) do
		rt = rt + v
	end
end

table.join = function (a, b)
    local ret = {}
    for k,v in pairs(a) do
        ret[k] = v
    end
    for k,v in pairs(b) do
        ret[k] = v
    end
    return ret
end

table.dump = function (a)
	print ("a = {")
	for k,v in pairs(a) do
		print (k, "=", v, ",")
	end
	print ("}")
end

function lerp(current, start, finish, startval, endval)
	return (current - start) * (endval - startval) / (finish - start) + startval
end

function clerp(c, s, f, sv, ev)
	return clamp(lerp(c,s,f,sv,ev), sv, ev)
end

function mix(r, s, e)
	return lerp(r, 0, 1, s, e)
end

function cmix(r, s, e)
	return clerp(r, 0, 1, s, e)
end

function fract(d)
	return d - floor(d)
end

function with(obj, t)
		if type(t) == "table" then
	    for k,v in pairs(t) do
	        obj[k] = v
	    end
		end
    return obj
end

function map(f, t)
	local ret = {}
	for k,v in pairs(t) do
		ret[k] = f(v)
	end
	return ret
end

--[[
	Warning: using pairs (underlying on the with)
	does not guarantee order. If you're setting image with
	ScreenObject or with, you could potentially end up
	side effect'd at the wrong order.
]]
function ScreenObject(t)
	local x = Engine:CreateObject()
  return with(x, t)
end

function AdjustInBox(transform, params)
    params = params or {x = 0, y = 0, w = ScreenWidth, h = ScreenHeight}
	local x = params.x or 0
	local y = params.y or 0
	local w = params.w or ScreenWidth
	local h = params.h or ScreenHeight
	local oldWidth = transform.Width
	local oldHeight = transform.Height
	local Background = transform or params.Background

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

librd = {
	make_new = function (t, initializer)
        assert(initializer)
				t.__index = t
				t.new = function (self, rt)
									local ret = {}
									setmetatable(ret, self)
									with(t, rt)
									initializer(ret)
									return ret
							 end
			return t
		end,
	intToDigits = function(i, base)
		local b = base or 10
		local ret = {}
		i = floor(i)
		while i >= 1 do
			local rem = floor(i) % b
			table.insert(ret, 1, rem)
			i = floor(i / 10)
		end
    return ret
	end
}

return librd
