function getMoveFunction(sX, sY, eX, eY, obj)
	return function(frac)
		obj.X = sX + (eX - sX)*frac 
		obj.Y = sY + (eY - sY)*frac
		return 1
	end
end

function getUncropFunction(w, h, iw, ih, obj)
	return function(frac)
		obj.Width = w 
		obj.Height = h*(1-frac)
		obj:SetCropByPixels(0, iw, (ih - (ih*frac*0.5)), ih*frac*0.5)
		return 1
	end
end

function getFadeFunction(sF, eF, obj)
	return function (frac)
		obj.Alpha = sF + (eF - sF)*frac
		return 1
	end
end