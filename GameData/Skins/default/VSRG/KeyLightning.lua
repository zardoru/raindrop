HitLightning = {}

HitLightning.OffTime = {}

HitLightning.Times = {}

HitLightning.Height = 250

function LightFunction () if GetConfigF("DisableHitlightningAnimation", "") == 1 then return 0 else return 1 end end

HitLightning.Enabled = GetConfigF("DisableHitlightning", "") ~= 0
HitLightning.Animate = LightFunction()

HitLightning.Pressed = {}
HitLightning.Position = {}

HitLightning.Image = "VSRG/hitlightning.png"

function HitLightning.Init()

	if HitLightning.Enabled == 0 then
		return
	end

	for i = 1, Channels do
		HitLightning.Times[i] = 1
		HitLightning.Pressed[i] = 0
	end

	for i=1, Channels do
		HitLightning[i] = Engine:CreateObject()

		HitLightning.OffTime[i] = 1
		
		HitLightning[i].Image = HitLightning.Image
		HitLightning[i].Centered = 1
		HitLightning[i].BlendMode = BlendAdd
		
		HitLightning[i].Width = GetConfigF("Key" .. i .. "Width", ChannelSpace)
		HitLightning[i].Height = HitLightning.Height

		local scrollY = 0
		local scrollX = 0
		if Upscroll ~= 0 then
			scrollX = GetConfigF("Key" .. i .. "X", ChannelSpace);
			scrollY = GearHeight +  HitLightning.Height / 2
			HitLightning[i].Rotation = (180)
		else
			scrollX = GetConfigF("Key" .. i .. "X", ChannelSpace);
			scrollY = ScreenHeight - GearHeight - HitLightning.Height / 2
		end

		HitLightning.Position[i] = { x = scrollX, y = scrollY }
		HitLightning[i].X = scrollX
		HitLightning[i].Y = scrollY
		HitLightning[i].Layer = 15
		HitLightning[i].Alpha = 0
	end
end

function HitLightning.LanePress(Lane, IsKeyDown, SetRed)

	if HitLightning.Enabled == 0 then
		return
	end

	if CurrentSPB ~= math.huge then
		HitLightning.OffTime[Lane+1] = CurrentSPB / 1.5
	end

	if HitLightning.OffTime[Lane+1] > 3 then
		HitLightning.OffTime[Lane+1] = 3
	end	

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

		if HitLightning.Pressed[i] == 0 then
			if HitLightning.Times[i] <= HitLightning.OffTime[i] then
				if HitLightning.Animate == 1 then
					local Lerping = math.pow(HitLightning.Times[i] / HitLightning.OffTime[i], 2)
					local Additive
					HitLightning[i].ScaleX = 1 - Lerping
					HitLightning[i].ScaleY = 1 + 1.5 * Lerping

					Additive = HitLightning.Height / 2 * 1.5 * Lerping

					if Upscroll ~= 0 then
						Additive = Additive * -1
					end

					HitLightning[i].Y = HitLightning.Position[i].y - Additive
					HitLightning[i].Alpha = ( 1 - Lerping )
				elseif HitLightning.Animate == 2 then
					local Lerping = math.pow(HitLightning.Times[i] / HitLightning.OffTime[i], 2)
					local Additive = 0

					Additive = HitLightning.Height / 2 * Lerping
					HitLightning[i].ScaleY = 1 - Lerping
					HitLightning[i].Y = HitLightning.Position[i].y + Additive
					HitLightning[i].Alpha = 1
				end
			else
				HitLightning[i].Alpha = 0
			end

		else
			HitLightning[i]:SetScale(1)
			HitLightning[i].Alpha = 1
			HitLightning[i].Y = HitLightning.Position[i].y
		end

	end
end