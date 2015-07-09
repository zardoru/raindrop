fallback_require("VSRG/GameplayObjects")

local distance = 100

if Upscroll == 0 then
	JudY = ScreenHeight - GearHeight - distance
else
	JudY = GearHeight + distance
end

Judgment.Position.y = JudY
Judgment.Table[6] = "judge-excellentb.png"
Judgment.Scale = 0.7
Judgment.ScaleHit = 0.9
Judgment.ScaleMiss = 0.5
Judgment.ShowTimingIndicator = 0
Judgment.Tilt = 0

Lifebar.FillOffset = -13
Lifebar.FillWidth = 12
