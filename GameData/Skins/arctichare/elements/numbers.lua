game_require "TextureAtlas"
skin_require "skin_defs"

local DigitalNumbers = {
	construct = function(self, x, y, isBig, digits)
		self.isBig = isBig

		if self.isBig then
			self.atlas = TextureAtlas:skin_new("assets/digits-big.csv")
			self.width = 70 * SkinScale
			self.height = 120 * SkinScale
		else
			self.atlas = TextureAtlas:skin_new("assets/digits-small.csv")
			self.width = 40 * SkinScale
			self.height = 70 * SkinScale
		end

		self.x = x
		self.y = y

		self.digits = digits
		self.objects = {}

		for i=1, self.digits do
			self.objects[i] = ScreenObject {
				X = self.x + self.width * (i - 1),
				Y = self.y,
				Layer = 21
			}

			self.objects[i].Texture = "assets/" .. self.atlas.File
			self.objects[i].Width = self.width
			self.objects[i].Height = self.height
		end

		self:update(0)
	end,
	new = function(self, x, y, isBig, digits)
		local ret = {}
		setmetatable(ret, self)
		ret:construct(x, y, isBig, digits)
		return ret
	end,
	update = function(self, num, zeropad)
		if num == nil then 
			return
		end
		
		local s = tostring(num)
		if #s > self.digits then
			s = string.sub(s, #s - self.digits + 1)
		elseif #s < self.digits then
			if zeropad then
				s = string.rep("0", self.digits - #s) .. s
			else
				s = string.rep("X", self.digits - #s) .. s
			end
		end

		

		for i=1, self.digits do
			local c = string.sub(s, i, i)
			self.atlas:SetObjectCrop(self.objects[i], "digits-" .. c .. ".png")
		end
	end
}

DigitalNumbers.__index = DigitalNumbers
return DigitalNumbers
