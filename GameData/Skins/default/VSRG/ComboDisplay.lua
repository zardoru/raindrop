
if Upscroll == 0 then
	cY = ScreenHeight/4
else
	cY = ScreenHeight*3/4
end

ComboDisplay = {
	DigitWidth = 50,
	DigitHeight = 50,

	Position = {
		x = GearStartX + GearWidth / 2,
		y = cY
	},

	BumpTime = 0,
	BumpTotalTime = 0.1, -- Time the animation runs
	BumpFactor = 1.3,
	BumpVertically = 1,
	BumpHorizontally = 1,
	HoldBumpFactor = 1.2,
	ExNotifyImg = "VSRG/combo_bonus.png",
	ExNotifyTime = 0.34,
	ExNotifyExtraBump = 0.5
}

function ComboDisplay.SetName(i)
	return i-1 .. ".png"
end

-- Internal Constants
ComboDisplay.Targets = {}
ComboDisplay.Images = {}

ComboDisplay.BumpMiss = 1
ComboDisplay.BumpHit = 2
ComboDisplay.BumpHold = 3
ComboDisplay.BumpInactive = 0

ComboDisplay.BumpColor = 0

ComboDisplay.BumpKind = ComboDisplay.BumpInactive

ComboDisplay.BumpRealTotalTime = ComboDisplay.BumpTotalTime

ComboDisplay.Digits = {}
ComboDisplay.ExNotifyObject = nil

function ComboDisplay.Init()

	ComboDisplay.Atlas = TextureAtlas:new(Obj.GetSkinFile("VSRG/combosheet.csv"))

	for i = 1, 6 do -- Drawing targets
		ComboDisplay.Targets[i] = Obj.CreateTarget()
		Obj.SetTarget(ComboDisplay.Targets[i])
		Obj.SetImageSkin("VSRG/"..ComboDisplay.Atlas.File)
		Obj.SetCentered(1)
		Obj.SetZ(24)
		Obj.SetAlpha(0)
	end

	for i = 1, 10 do -- Digit images
		ComboDisplay.Images[i] = ComboDisplay.SetName(i)
	end

	ComboDisplay.ExNotifyObject = Obj.CreateTarget()
	Obj.SetTarget(ComboDisplay.ExNotifyObject)
	Obj.SetCentered(1)
	Obj.SetZ(24)
	Obj.SetAlpha(0)
	Obj.SetImageSkin(ComboDisplay.ExNotifyImg)

	ComboDisplay.ExNotifyCurTime = 0

end

function ComboDisplay.Cleanup()
	for i = 1, 6 do
		Obj.CleanTarget(ComboDisplay.Targets[i])
	end
end

function ComboDisplay.Update()

	ComboDisplay.Digits = {}

	local TCombo = Combo
	local digt = 0

	while TCombo >= 1 do
		table.insert(ComboDisplay.Digits, math.floor(TCombo) % 10)
		TCombo = TCombo / 10
		digt = digt + 1
	end

	-- active digits = #Digits
	local ActDig = #ComboDisplay.Digits + 1
	local Size = { w = ComboDisplay.DigitWidth, h = ComboDisplay.DigitHeight }
	local DisplaySize = (digt-1) * Size.w/2
	local Position = ComboDisplay.Position
	ComboDisplay.ExNotifyPos = {
		x = Position.x + DisplaySize + Size.w/2,
		y = Position.y - ComboDisplay.DigitHeight/2
	}

	for i = 1, 6 do
		local NewPosition = { x = Position.x + DisplaySize - (i-1) * Size.w, y = Position.y }
		Obj.SetTarget(ComboDisplay.Targets[i])

		if i < ActDig then
			local Tab = ComboDisplay.Atlas.Sprites[ComboDisplay.Images[ComboDisplay.Digits[i]+1]]

			Obj.CropByPixels(Tab.x, Tab.y, Tab.x+Tab.w, Tab.y+Tab.h)
			Obj.SetSize(Size.w, Size.h)
			Obj.SetAlpha(1)

			Obj.SetPosition(NewPosition.x, NewPosition.y)
		else
			Obj.SetAlpha(0)
		end
	end
end

function ComboDisplay.Hit(Kind)

	ComboDisplay.BumpTime = 0
	ComboDisplay.BumpKind = BumpHit
	ComboDisplay.BumpColor = Kind

	if ComboDisplay.BumpColor ~= 0 then
		ComboDisplay.ExNotifyCurTime = 0
	end

	ComboDisplay.Update()

end

function ComboDisplay.Miss()

	ComboDisplay.BumpTime = 0
	ComboDisplay.BumpKind = BumpMiss
	ComboDisplay.Update()

end

function ComboDisplay.Run(Delta)

	local Ratio = 1 - (Beat - math.floor(Beat))

	Obj.SetTarget(ComboDisplay.ExNotifyObject)

	-- the +2 at the topright
	if #ComboDisplay.Digits ~= 0 then
		
		if ComboDisplay.BumpColor ~= 0 then

			Obj.SetPosition(ComboDisplay.ExNotifyPos.x, ComboDisplay.ExNotifyPos.y)

			local Factor = 1 + ComboDisplay.ExNotifyExtraBump * Ratio			
			Obj.SetScale(Factor, Factor)
		
		else -- Time only runs if we're not at an "AWESOME" hit.
			ComboDisplay.ExNotifyCurTime = ComboDisplay.ExNotifyCurTime + Delta
		end

		local Ratio = math.max(1 - ComboDisplay.ExNotifyCurTime / ComboDisplay.ExNotifyTime, 0)
		Obj.SetAlpha (Ratio)

	else
		Obj.SetAlpha(0)
	end

	local RebootHT = 0
	for i=1,Channels do
	   if HeldKeys[i] ~= 0 then
		RebootHT = 1
		break
	   end
	end

	local HoldScale = 1

	if RebootHT == 1 then
		local BT = Beat * 2
		local Ratio = (BT - math.floor(BT))
		HoldScale = ComboDisplay.HoldBumpFactor - Ratio * (ComboDisplay.HoldBumpFactor - 1)
	end

	local newScaleRatio = 1

	-- the numbers themselves
	if ComboDisplay.BumpKind ~= 0 then
		ComboDisplay.BumpTime = ComboDisplay.BumpTime + Delta

		local Ratio = ComboDisplay.BumpTime / ComboDisplay.BumpRealTotalTime

		if Ratio >= 1 then
			ComboDisplay.BumpKind = ComboDisplay.BumpInactive;
		end

		newScaleRatio = (ComboDisplay.BumpFactor - (Ratio * math.abs(ComboDisplay.BumpFactor - 1))) * HoldScale
	end

	for i= 1, 6 do

		Obj.SetTarget(ComboDisplay.Targets[i])
		local scaleX = 1
		local scaleY = 1
		local usedScale
		
		if ComboDisplay.BumpKind ~= ComboDisplay.BumpInactive then
			usedScale = newScaleRatio
		else
			usedScale = HoldScale
		end
		
		if ComboDisplay.BumpHorizontally ~= 0 then
			scaleX = usedScale
		end
			
		if ComboDisplay.BumpVertically ~= 0 then
			scaleY = usedScale
		end
		
		Obj.SetScale (scaleX, scaleY)

		if ComboDisplay.BumpColor ~= 0 then
			Obj.SetColor(1, 2.5, 2.5)
		else
			Obj.SetColor(1, 1, 1)
		end
	
	end

end

