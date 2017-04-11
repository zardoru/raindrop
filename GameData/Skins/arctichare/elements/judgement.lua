skin_require "skin_defs"
skin_require "custom_defs"
game_require "TextureAtlas"

local Judgment = {
	JudgeMap = {
		"judge-perfect.png", -- w1 etc
		"judge-great.png",
		"judge-good.png",
		"judge-bad.png",
		"judge-miss.png", -- w5 is unused, repeat miss
		"judge-miss.png"
	},
	ComboMap = {
		"perfect",
		"great",
		"good",
		"bad",
		"miss"
	},
	construct = function(self, player_number)
		self.Transform = Transformation()

		self.JudgeAtlas = TextureAtlas:skin_new("assets/judge.csv")
		self.ComboAtlas = TextureAtlas:skin_new("assets/judge-combo.csv")
		self.PGreatAtlas = TextureAtlas:skin_new("assets/judge-pgreat.csv")
		self.PacemakerGoodAtlas = TextureAtlas:skin_new("assets/pace-a.csv")
		self.PacemakerBadAtlas = TextureAtlas:skin_new("assets/pace-b.csv")

		-- Judgement object
		self.Judgment = ScreenObject {
			Centered = 1,
			Alpha = 0,
			Layer = 20,
			ChainTransformation = self.Transform
		}

		-- force load if not preloaded
		self.Judgment.Texture = "assets/" .. self.JudgeAtlas.File
		self.Judgment.Texture = "assets/" .. self.PGreatAtlas.File
		ScaleObj(self.Judgment)

		-- Combo object
		self.ComboDigits = {}

		local ScoreKeeper = Game:GetPlayer(0).Scorekeeper
		local max_digits = math.floor(math.log10(ScoreKeeper.MaxNotes)) + 1
		for i=1, max_digits do
			local obj = ScreenObject {
				Alpha = 0,
				ChainTransformation = self.Transform,
				Texture = "assets/" .. self.ComboAtlas.File,
				Layer = self.Judgment.Layer
			}
			ScaleObj(obj)
			self.ComboDigits[#self.ComboDigits + 1] = obj
		end

		self.DigitCount = max_digits

		-- Pacemaker object

		-- Fast/Slow indicator
		self.FSIndicator = ScreenObject {
			Alpha = 0,
			Centered = 1,
			Layer = 20,
			ChainTransformation = self.Transform
		}

		ScaleObj(self.FSIndicator)

		self.TimeToFade = 0.25
		self.TimeRemainingToFade = 0
		self.FadeDuration = 0.1
		self.TimeFading = 0
		self.TimePerPGreatFrame = 0.02

		self.BlinkTime = 0

		self.ScaleFeedbackTime = 0.07
		self.TimeScaling = 0
	end,
	new = function(self, player_number)
		local ret = {}
		setmetatable(ret, self)
		ret:construct(player_number)
		return ret
	end,
	onJudge = function(self, judge, timeoff)
		-- update main judge
		self.LastJudge = judge
		if judge == 1 or judge == 0 then
			-- if pgreat or judge == 0 then
			self.Judgment.Texture = "assets/" .. self.PGreatAtlas.File
			self.PGreatAtlas:SetObjectCrop(self.Judgment, "judge-pgreat-1.png", true)
			--end
		else
			self.Judgment.Texture  = "assets/" .. self.JudgeAtlas.File
			self.JudgeAtlas:SetObjectCrop(self.Judgment, Judgment.JudgeMap[judge], true)
		end
		self.Judgment.Alpha = 1
		self.TimeScaling = self.ScaleFeedbackTime
		self.TimeRemainingToFade = self.TimeToFade

		-- update pacemaker

		-- update F/S indicator
		if judge == 1 or judge == 0 then
			self.FSIndicator.Alpha = 0
		else
			self.FSIndicator.Alpha = 1
			if timeoff > 0 then
				self.FSIndicator.Texture = "assets/rate-slow.png"
			else
				self.FSIndicator.Texture = "assets/rate-fast.png"
			end

			local jh = self.Judgment.Height * SkinScale / 2
			local ih = self.FSIndicator.Height * SkinScale / 2
			self.FSIndicator.Y = self.Judgment.Y - jh - ih
		end
	end,
	update = function(self, delta)
		local ScoreKeeper = Game:GetPlayer(0).Scorekeeper
		self.TimeScaling = self.TimeScaling - delta
		self.TimeRemainingToFade = self.TimeRemainingToFade - delta
		self.BlinkTime = self.BlinkTime + delta

		self.Transform.X = GearStartX - 40 * SkinScale + GearWidth / 2
		self.Transform.Y = 1560 * SkinScale / 2

		if self.TimeScaling > 0 then
			local ys = lerp(self.TimeScaling, self.ScaleFeedbackTime, 0, 1.25, 1.0)
			local xs = lerp(self.TimeScaling, self.ScaleFeedbackTime, 0, 1.25, 1.0)
			self.Transform.ScaleX = xs
			self.Transform.ScaleY = ys
		else
			self.Transform.ScaleX = 1.0
			self.Transform.ScaleY = 1.0
		end

		local blink_frame
		if self.LastJudge == 1 or self.LastJudge == 0 then
			local cycleTime = math.fmod(self.BlinkTime, self.TimePerPGreatFrame * 6)
			local frame = math.floor(cycleTime / self.TimePerPGreatFrame + 1)
			blink_frame = frame
			self.PGreatAtlas:SetObjectCrop(self.Judgment, "judge-pgreat-" .. frame .. ".png", true)
		else
			blink_frame = self.LastJudge
		end

				-- Blink and alpha stuff.
		if blink_frame and blink_frame ~= 6 then
			local combo_str = tostring(ScoreKeeper:GetScore(ST_COMBO))
			local digit_w = 70
			local combo_w = #combo_str * digit_w * SkinScale


			local judge_space = -15
			local digit_offset = self.Judgment.Width / 2 * SkinScale + judge_space
			-- combo digits plus judge center
			local dpj = (combo_w + judge_space + self.Judgment.Width * SkinScale) / 2

			local joffset =  (dpj - self.Judgment.Width / 2 * SkinScale)

			if combo_w > 0 then
				self.Judgment.X = -joffset
			end

						-- Place F/S on top of combo
						local jstart = self.Judgment.X + self.Judgment.Width / 2 * SkinScale
						self.FSIndicator.X = jstart

			for i=1, #combo_str do
				if i > self.DigitCount then
					break
				end

				local di = #combo_str - (i - 1)
				local digit = string.sub(combo_str, di, di)

				local spr = "judge-" .. Judgment.ComboMap[blink_frame] .. "-combo-" .. digit .. ".png"
				self.ComboAtlas:SetObjectCrop(self.ComboDigits[di], spr, true)
				with ( self.ComboDigits[di], {
					Y = -self.Judgment.Height / 2 * SkinScale,
					X = digit_offset + combo_w - digit_w * i * SkinScale - joffset,
					Alpha = 1
				})
			end

			for i=(#combo_str+1), self.DigitCount do
				self.ComboDigits[i].Alpha = 0
			end
		end
	end
}

Judgment.__index = Judgment
return Judgment
