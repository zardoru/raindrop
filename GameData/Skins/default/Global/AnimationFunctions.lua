function getMoveFunction(sX, sY, eX, eY)
	return function(frac)
		Obj.SetPosition(sX + (eX - sX)*frac, sY + (eY - sY)*frac)
		return 1
	end
end

function getUncropFunction(w, h, iw, ih)
	return function(frac)
		Obj.SetSize(w, h*(1-frac))
		Obj.CropByPixels(0, ih*frac*0.5, iw, (ih - (ih*frac*0.5)))
		return 1
	end
end

function getFadeFunction(sF, eF)
	return function (frac)
		Obj.SetAlpha(sF + (eF - sF)*frac)
		return 1
	end
end