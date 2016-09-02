skin_require "skin_defs"
Gauge = skin_require "elements.gauge"
Numbers = skin_require "elements.numbers"
Lightup = skin_require "elements.lightup"
Judgment = skin_require "elements.judgement"

local Components = {}
function Components:Init()
    self.BPM = Numbers:new(1814 * SkinScale, 1800 * SkinScale, true, 3)
    self.Mins = Numbers:new(1830 * SkinScale, 2060 * SkinScale, false, 2)
    self.Secs = Numbers:new(1930 * SkinScale, 2060 * SkinScale, false, 2)

    if PlayerSide == 1 then
        self.Score = Numbers:new(880 * SkinScale, 40 * SkinScale, true, 6)
        self.MulLeft = Numbers:new(920 * SkinScale, 1720 * SkinScale, true, 2)
        self.MulRight = Numbers:new(1090 * SkinScale, 1720 * SkinScale, true, 2)
        self.Gauge = Gauge:new(970 * SkinScale, 1434 * SkinScale, 1, true)

        self.J1 = Numbers:new(30 * SkinScale, 2060 * SkinScale, false, 4)
        self.J2 = Numbers:new(250 * SkinScale, 2060 * SkinScale, false, 4)
        self.J3 = Numbers:new(470 * SkinScale, 2060 * SkinScale, false, 4)
        self.J4 = Numbers:new(690 * SkinScale, 2060 * SkinScale, false, 4)
        self.JM = Numbers:new(910 * SkinScale, 2060 * SkinScale, false, 4)
        self.MC = Numbers:new(1130 * SkinScale, 2060 * SkinScale, false, 4)

        self.Lightup = Lightup:new(1)
        self.Judgment = Judgment:new(1)
    else
				self.Score = Numbers:new(2540 * SkinScale, 40 * SkinScale, true, 6)
				self.MulLeft = Numbers:new(2570 * SkinScale, 1720 * SkinScale, true, 2)
				self.MulRight = Numbers:new(2740 * SkinScale, 1720 * SkinScale, true, 2)
				self.Gauge = Gauge:new(970 * SkinScale, 1550 * SkinScale, 2, true)

				self.J1 = Numbers:new(2550 * SkinScale, 2060 * SkinScale, false, 4)
				self.J2 = Numbers:new(2770 * SkinScale, 2060 * SkinScale, false, 4)
				self.J3 = Numbers:new(2990 * SkinScale, 2060 * SkinScale, false, 4)
				self.J4 = Numbers:new(3210 * SkinScale, 2060 * SkinScale, false, 4)
				self.JM = Numbers:new(3430 * SkinScale, 2060 * SkinScale, false, 4)
				self.MC = Numbers:new(3650 * SkinScale, 2060 * SkinScale, false, 4)

				self.Lightup = Lightup:new(2)
				self.Judgment = Judgment:new(2)
    end
end

function Components:Update(delta)
    self.BPM:update(CurrentBPM)

    local left = SongDuration - SongTime

    if left > 0 then
        local mins = math.floor(left / 60)
        local secs = math.floor(left - mins * 60)
        self.Mins:update(mins)
        self.Secs:update(secs, true)
    else
        self.Mins:update(0)
        self.Secs:update(0, true)
    end

    self.Score:update(ScoreKeeper:getScore(ST_EX))
    local int = math.floor(Game:GetUserMultiplier())
    local frac = math.floor((Game:GetUserMultiplier() - int) * 100)
    self.MulLeft:update(int)
    self.MulRight:update(frac, true)
    self.Gauge:update(delta)

    self.J1:update(ScoreKeeper:getJudgmentCount(SKJ_W1))
    self.J2:update(ScoreKeeper:getJudgmentCount(SKJ_W2))
    self.J3:update(ScoreKeeper:getJudgmentCount(SKJ_W3))
    self.J4:update(ScoreKeeper:getJudgmentCount(SKJ_W4))
    self.JM:update(ScoreKeeper:getJudgmentCount(SKJ_MISS))
    self.MC:update(ScoreKeeper:getScore(ST_MAX_COMBO))

    self.Lightup:update(delta, KeyArray)
    self.Judgment:update(delta)
end

function Components:OnHit(judge, timeoff)
    self.Judgment:onJudge(judge, timeoff)
end

function Components:OnMiss(judge, timeoff)
    self.Judgment:onJudge(judge, timeoff)
end

return Components
