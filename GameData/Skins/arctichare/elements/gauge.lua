skin_require "skin_defs"
skin_require "elements.numbers"

-- arctic hare beatmania gauge
local Gauge = {
	construct = function(self, x, y, a, on)
		self.ActivePlayerNumber = a
		print "===== Building Gauge ====="
		if on then
			self.ActivePlayer = ScreenObject {
				X = x,
				Y = y,
		  		Layer = 17
			}

			self.ActivePlayer.Texture = "assets/" .. Gauge.Atlas.File
			self.ActivePlayer.Height = 70
			
			-- player
			if a == 2 then
				Gauge.Atlas:SetObjectCrop(self.ActivePlayer, "gauge-player-2P.png")
			else
				Gauge.Atlas:SetObjectCrop(self.ActivePlayer, "gauge-player-1P.png")
			end

			ScaleObj(self.ActivePlayer)
		end

		self.GaugeBG = ScreenObject {
			X = x + 110 * SkinScale,
			Y = y,
			Layer = 16
		}

		local gt = Global:GetCurrentGaugeType(0)
		if gt == LT_GROOVE then
			self.GaugeBG.Texture = "assets/gauge-groove-off.png"
		elseif gt == LT_EASY or gt == LT_O2JAM or gt == LT_STEPMANIA then
			self.GaugeBG.Texture = "assets/gauge-green-off.png"
		else
			self.GaugeBG.Texture = "assets/gauge-survival-off.png"
		end

		ScaleObj(self.GaugeBG)

		print (on, self.GaugeBG.Texture, gt)
		if on then
			self.GaugeHealth = ScreenObject {
				X = x + 110 * SkinScale,
				Y = y,
				Layer = 17,
				Lighten = 1
			}

			if gt == LT_GROOVE then
				self.GaugeHealth.Texture = "assets/gauge-groove-on.png"
			elseif gt == LT_EASY or gt == LT_O2JAM or gt == LT_STEPMANIA then
				self.GaugeHealth.Texture = "assets/gauge-green-on.png"
			else
				self.GaugeHealth.Texture = "assets/gauge-survival-on.png"
			end

			ScaleObj(self.GaugeHealth)

			self.HealthBG = ScreenObject {
				X = x + 1740 * SkinScale,
				Y = y,
				Layer = 9
			}

			self.HealthBG.Texture = "assets/gauge-percent-on.png"
			ScaleObj(self.HealthBG)

			self.HealthPCT = Numbers:new(x + 1740 * SkinScale, y, false, 3)
		end



		self.IsOn = on
	end,
	new = function(self, x, y, active_player_number, is_on)
		local ret = {}
		setmetatable(ret, self)
		ret:construct(x, y, active_player_number, is_on)
		return ret
	end,
	update = function(self, delta)
		local health = Game:GetPlayer(0).LifebarPercent / 100
		local Beat = Game:GetPlayer(0).Beat
		if self.IsOn then
			local hp = math.floor(health * self.Steps)
			local hpPCT = math.floor(health * 100)
			local w = hp * Gauge.SkinPxPerStep
			local beatfx = 1 - (Beat - math.floor(Beat))

			self.GaugeHealth.Width = w
			self.GaugeHealth:SetCropByPixels(0, w, 0, self.GaugeHealth.Height)
			self.GaugeHealth.LightenFactor = beatfx

			self.HealthPCT:update(hpPCT)
		end
	end
}

Gauge.Atlas = TextureAtlas:skin_new("assets/gauge-player.csv")
Gauge.Steps = 80
Gauge.SkinPxPerStep = 20
Gauge.__index = Gauge
return Gauge
