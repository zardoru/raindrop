skin_require "skin_defs"
skin_require "custom_defs"

local Lightup = {
	construct = function(self, player_side)
		self.ActivePlayerNumber = player_side

		if player_side == 1 then
			self.white = "assets/lightup-1-white.png"
			self.black = "assets/lightup-1-black.png"
			self.scratch = "assets/lightup-1-scratch.png"
			self.coords = CP1
		elseif player_side == 2 then
			self.white = "assets/lightup-2-white.png"
			self.black = "assets/lightup-2-black.png"
			self.scratch = "assets/lightup-2-scratch.png"
			self.coords = CP2
		end

		local map = {0, 1, 2, 1, 2, 1, 2, 1}

		self.Lights = {}
		local sidecnt
		if Channels == 12 then
			sidecnt = 6
		elseif Channels == 16 then
			sidecnt = 8
		else
			sidecnt = Channels
		end

		for i=1, sidecnt do
			self.Lights[i] = Engine:CreateObject()

			local pic
			if map[i] == 0 then
				pic = self.scratch
			elseif map[i] == 1 then
				pic = self.white
			else
				pic = self.black
			end

			print("load ", pic)
			self.Lights[i].Texture = pic
			with(self.Lights[i], {
				Centered = 1,
				Y = self.Lights[i].Height / 2 * SkinScale,
				X = self.coords[i],
				Layer = 25,
				BlendMode = BlendAdd
			})

			self.Lights[i].Alpha = 0

			ScaleObj(self.Lights[i])
			print(self.Lights[i].X, self.Lights[i].Y)
		end
	end,
	new = function(self, player_side)
		local ret = {}
		setmetatable(ret, self)
		ret:construct(player_side)
		return ret
	end,
	update = function(self, delta, keystate)
		for k,v in ipairs(keystate) do
			if v then
				self.Lights[k].Alpha = 1
			else
				self.Lights[k].Alpha = 0
			end
		end
	end
}

Lightup.__index = Lightup
return Lightup
