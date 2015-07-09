fallback_require("VSRG/ScoreDisplay")

ScoreDisplay.DigitWidth = 40
ScoreDisplay.DigitHeight = 47
ScoreDisplay.Sheet = "VSRG/scoresheet.csv"

function ScoreDisplay.SetName(i)
	return "score-".. i-1 ..".png"
end