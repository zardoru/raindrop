fallback_require("VSRG/Explosions.lua")

Explosions.HitFramerate = 60
Explosions.HitFrames = 20
Explosions.HitScale = 1

Explosions.MissShow = 0


function Explosions.HitName (i)
	return "explosion-" .. i-1 .. ".png"
end