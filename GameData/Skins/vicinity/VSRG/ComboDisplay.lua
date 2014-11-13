fallback_require("VSRG/ComboDisplay.lua")

ComboDisplay.BumpHorizontally = 0
ComboDisplay.DigitWidth = 41
ComboDisplay.DigitHeight = 60

function ComboDisplay.SetName(i)
	return "default-" .. i-1 .. ".png"
end