
ProgressTick = { Image = "progress_tick.png" }
Pulse = { Image = "pulse_ver.png", Height = 200 }

function ProgressTick.Init()
	ProgressTick.Object = Obj.CreateTarget()

	-- When not active, Beat <= 0.
	print (Beat)
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
		Obj.SetAlpha(0.5*(1- (Beat - math.floor(Beat))))
	else
		Obj.SetAlpha(0)
	end
end
