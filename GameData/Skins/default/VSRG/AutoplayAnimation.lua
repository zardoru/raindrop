AutoAnimation = {}

function AutoAnimation.Init()
	AutoBN = Obj.CreateTarget()
	
	BnMoveFunction = getMoveFunction(GearStartX + GearWidth/2, -60, GearStartX + GearWidth/2, 100)
			
	Obj.SetTarget(AutoBN)
	Obj.SetImageSkin("VSRG/auto.png")

	Obj.SetCentered(1)

	Obj.AddAnimation( "BnMoveFunction", 0.75, 0, EaseOut )

	w, h = Obj.GetSize()
	factor = GearWidth / w * 3/4
	Obj.SetSize(w * factor, h * factor)
	Obj.SetZ(28)
	AutoFinishAnimation = getUncropFunction(w*factor, h*factor, w, h)
	RunAutoAnimation = true
end

function AutoAnimation.Finish()
	if AutoBN then
		Obj.SetTarget(AutoBN)
		Obj.AddAnimation ("AutoFinishAnimation", 0.35, 0, EaseOut)
		RunAutoAnimation = false
	end
end

function AutoAnimation.Cleanup()
	if AutoBN then
		Obj.CleanTarget (AutoBN)
	end
end

function AutoAnimation.Run(Delta)
	if AutoBN and RunAutoAnimation == true then
			local BeatRate = Beat / 2
			local Scale = math.sin( math.pi * 2 * BeatRate  )
			Scale = Scale * Scale * 0.25 + 0.75
			Obj.SetTarget(AutoBN)
			Obj.SetScale(Scale, Scale)
		end
end