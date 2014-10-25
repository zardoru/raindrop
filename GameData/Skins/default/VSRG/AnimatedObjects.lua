
ProgressTick = { Image = "VSRG/progress_tick.png" }
Pulse = { Image = "VSRG/pulse_ver.png", Height = 200 }
MissHighlight = {}

function ProgressTick.Init()
	ProgressTick.Object = Obj.CreateTarget()

	-- When not active, Beat <= 0.
	ProgressTick.BeatOffs = -Beat

	Obj.SetTarget(ProgressTick.Object)
	Obj.SetImageSkin(ProgressTick.Image)
	Obj.SetPosition( GearStartX - 5 - 16, 0 )
	Obj.SetZ(25)
end

function ProgressTick.Cleanup()
	Obj.CleanTarget(ProgressTick.Object)
end

function ProgressTick.Run(Delta)
	if Active ~= 0 then
		local Ratio = (Beat + ProgressTick.BeatOffs) / (SongDurationBeats + ProgressTick.BeatOffs)
		if SongTime > 0 then
			Obj.SetAlpha(1)
			Obj.SetPosition( GearStartX - 5 - 16, Ratio * (ScreenHeight - 16) )
		else
			Obj.SetAlpha(1 - SongTime / -1.5, 2)
			Obj.SetPosition( GearStartX - 5 - 16, (ScreenHeight - 16) * math.pow(SongTime / -1.5, 2) )
		end
	else
		Obj.SetAlpha( 0 )
	end
end

function Pulse.Init()
	Pulse.Object = Obj.CreateTarget()

	Obj.SetTarget(Pulse.Object)
	Obj.SetImageSkin(Pulse.Image)
	Obj.SetBlendMode(BlendAdd)

	if Upscroll ~= 0 then
		Obj.SetRotation(180)
		Obj.SetPosition(GearStartX + GearWidth, GearHeight + Pulse.Height )
	else
		Obj.SetPosition(GearStartX, ScreenHeight - GearHeight - Pulse.Height )
	end

	Obj.SetSize(GearWidth, Pulse.Height)
	Obj.SetZ(11)
	Obj.SetAlpha(0)
end

function Pulse.Cleanup()
	Obj.CleanTarget(Pulse.Object)
end

function Pulse.Run(Delta)
	if Active ~= 0 then
		Obj.SetAlpha(1 - (Beat - math.floor(Beat)))
	else
		Obj.SetAlpha(0)
	end
end

function MissHighlight.Init()

	MissHighlight.Time = {}
	MissHighlight.CurrentTime = {}

	for i=0, Channels do
		MissHighlight[i] = Obj.CreateTarget()
		Obj.SetTarget(MissHighlight[i])
		Obj.SetCentered(1)
		Obj.SetImageSkin("VSRG/miss_highlight.png")
		Obj.SetPosition(GetConfigF("Key"..i.."X", ChannelSpace), ScreenHeight/2)
		Obj.SetSize(GetConfigF("Key"..i.."Width", ChannelSpace), ScreenHeight)
		Obj.SetAlpha(0)
		Obj.SetZ(15)
		
		MissHighlight.Time[i] = 1 
		MissHighlight.CurrentTime[i] = 1
	end
end

function MissHighlight.Run(Delta)
	for i=0, Channels do
		Obj.SetTarget(MissHighlight[i])
		MissHighlight.CurrentTime[i] = MissHighlight.CurrentTime[i] + Delta
		
		if MissHighlight.CurrentTime[i] < MissHighlight.Time[i] then
			local Lerp = 1 - MissHighlight.CurrentTime[i] / MissHighlight.Time[i]
			Obj.SetAlpha(Lerp)
		else
			Obj.SetAlpha(0)
		end
	end
end

function MissHighlight.Cleanup()
	for i=0, Channels do
		Obj.CleanTarget(MissHighlight[i])
	end
end

function MissHighlight.OnMiss(Lane)
	MissHighlight.CurrentTime[Lane] = 0

	if CurrentSPB ~= math.huge then
		MissHighlight.Time[Lane] = CurrentSPB / 2
	end

end
