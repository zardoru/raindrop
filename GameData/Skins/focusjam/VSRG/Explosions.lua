fallback_require("VSRG/Explosions")

Explosions.HitFramerate = 60
Explosions.HitFrames = 20
Explosions.HitScale = 1

Explosions.MissShow = 0


print ("FOCUSJAM EXPLOSIONS EXECUTION")

function Explosions.HitName (i)
	return "explosion-" .. i-1 .. ".png"
end