HitLightning = {}

HitLightning.OffTime = {}

HitLightning.Times = {}

HitLightning.Height = 250

HitLightning.Enabled = GetConfigF("DisableHitlightning", "") ~= 0
HitLightning.Animate = GetConfigF("DisableHitlightningAnimation", "") ~= 0

HitLightning.Pressed = {}
HitLightning.Position = {}

HitLightning.Image = "hitlightning.png"

function HitLightning.Init()

	if HitLightning.Enabled == 0 then
		return
	end

	for i = 1, Channels do
		HitLightning.Times[i] = 1
		HitLightning.Pressed[i] = 0
	end

	for i=1, Channels do
		HitLightning[i] = Obj.CreateTarget()

		HitLightning.OffTime[i] = 1

		Obj.SetTarget(HitLightning[i])
		Obj.SetImageSkin(HitLightning.Image)
		Obj.SetCentered(1)
		Obj.SetBlendMode(BlendAdd)

		Obj.SetSize(GetConfigF("Key" .. i .. "Width", ChannelSpace), HitLightning.Height)

		local scrollY = 0
		local scrollX = 0
		if Upscroll ~= 0 then
			scrollX = GetConfigF("Key" .. i .. "X", ChannelSpace);
			scrollY = GearHeight +  HitLightning.Height / 2
			Obj.SetRotation(180)
		else
			scrollX = GetConfigF("Key" .. i .. "X", ChannelSpace);
			scrollY = ScreenHeight - GearHeight - HitLightning.Height / 2
		end

		HitLightning.Position[i] = { x = scrollX, y = scrollY }
		Obj.SetPosition(scrollX, scrollY)
		Obj.SetZ(15)
		Obj.SetAlpha(0)
	end
end

function HitLightning.LanePress(Lane, IsKeyDown, SetRed)

	if HitLightning.Enabled == 0 then
		return
	end

	HitLightning.OffTime[Lane+1] = CurrentSPB / 4

	if IsKeyDown == 0 then
		HitLightning.Times[Lane+1] = 0
		HitLightning.Pressed[Lane+1] = 0
	else
		HitLightning.Pressed[Lane+1] = 1
	end
end

function HitLightning.Run(Delta)

	if HitLightning.Enabled == 0 then
		return
	end

	for i=1, Channels do

		HitLightning.Times[i] = HitLightning.Times[i] + Delta

		Obj.SetTarget(HitLightning[i])

		if HitLightning.Pressed[i] == 0 then

			if HitLightning.Times[i] <= HitLightning.OffTime[i] and HitLightning.Animate ~= 0 then

				local Lerping = math.pow(HitLightning.Times[i] / HitLightning.OffTime[i], 2)

				Obj.SetScale(1 - Lerping, 1 + 1.5 * Lerping)

				local Additive = 0

				Additive = HitLightning.Height / 2 * 1.5 * Lerping

				if Upscroll ~= 0 then
					Additive = Additive * -1
				end

				Obj.SetPosition( HitLightning.Position[i].x, HitLightning.Position[i].y - Additive)
				Obj.SetAlpha( 1 - Lerping )
			else
				Obj.SetAlpha( 0 )
			end

		else
			Obj.SetTarget(HitLightning[i])
			Obj.SetScale( 1, 1 )
			Obj.SetPosition( HitLightning.Position[i].x, HitLightning.Position[i].y)
			Obj.SetAlpha ( 1 )
		end

	end
end

function HitLightning.Cleanup()

	if HitLightningEnabled == 0 then
		return
	end

	for i=1, Channels do
		Obj.CleanTarget(HitLightning[i])
	end

end
