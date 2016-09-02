AutoAnimation = {}

function AutoAnimation.Init()
	AutoBN = Engine:CreateObject()
	
	BnMoveFunction = getMoveFunction(GearStartX + GearWidth/2, -60, GearStartX + GearWidth/2, 100, AutoBN)
			
	AutoBN.Image = "VSRG/auto.png"

	AutoBN.Centered = 1

	Engine:AddAnimation(AutoBN, "BnMoveFunction", EaseOut, 0.75, 0 )

	w = AutoBN.Width
	h = AutoBN.Height
	
	factor = 350 / w * 3/4
	AutoBN.Width = w * factor
	AutoBN.Height = h * factor
	AutoBN.Layer = 28
	AutoFinishAnimation = getUncropFunction(w*factor, h*factor, w, h, AutoBN)
	RunAutoAnimation = true
end

function AutoAnimation.Finish()
	if AutoBN then
		Engine:AddAnimation (AutoBN, "AutoFinishAnimation", EaseOut, 0.35, 0)
		RunAutoAnimation = false
	end
end

function AutoAnimation.Run(Delta)
	if AutoBN and RunAutoAnimation == true then
			local BeatRate = Beat / 2
			local Scale = math.sin( math.pi * 2 * BeatRate )
			Scale = Scale * Scale * 0.25 + 0.75
			AutoBN:SetScale(Scale, Scale)
		end
end