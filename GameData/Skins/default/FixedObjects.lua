
Filter = { Image = "filter.png" }
JudgeLine = { Image = "judgeline.png" }
StageLines = { ImageLeft = "stage-left.png", ImageRight = "stage-right.png" }

function Filter.Init()
	if GetConfigF("ScreenFilter", "") == 0 then
		return
	end

	Filter.Object = Obj.CreateTarget()

	Obj.SetTarget(Filter.Object)

	Obj.SetImageSkin(Filter.Image)
	Obj.SetPosition(GearStartX, 0)
	Obj.SetSize(GearWidth, ScreenHeight)
	Obj.SetAlpha(0.7)
	Obj.SetZ(1)
end

function Filter.Cleanup()
	if GetConfigF("ScreenFilter", "") ~= 0 then
		Obj.CleanTarget(Filter.Object)
	end
end


function JudgeLine.Init()
	JudgeLine.Object = Obj.CreateTarget()
	JudgeLine.Size = { w = GearWidth, h = GetConfigF("NoteHeight", "") }

	Obj.SetTarget(JudgeLine.Object)
	Obj.SetImageSkin(JudgeLine.Image)
	Obj.SetCentered(1)

	if Upscroll ~= 0 then
		Obj.SetPosition (GearStartX + GearWidth / 2, JudgmentLineY)
	else
		Obj.SetPosition (GearStartX + GearWidth / 2, JudgmentLineY)
	end

	Obj.SetSize(JudgeLine.Size.w, JudgeLine.Size.h)
	Obj.SetZ(12)
end

function JudgeLine.Cleanup()
	Obj.CleanTarget (JudgeLine.Object)
end

function StageLines.Init()
	StageLines.Left = Obj.CreateTarget()
	StageLines.Right = Obj.CreateTarget()

	Obj.SetTarget(StageLines.Left)
	Obj.SetImageSkin(StageLines.ImageLeft)
	Obj.SetPosition(GearStartX - Obj.GetSize(), 0)
	Obj.SetSize(Obj.GetSize(), ScreenHeight)
	Obj.SetZ(20)

	Obj.SetTarget(StageLines.Right)
	Obj.SetImageSkin(StageLines.ImageRight)
	Obj.SetPosition(GearStartX + GearWidth, 0)
	Obj.SetSize(Obj.GetSize(), ScreenHeight)
	Obj.SetZ(20)
end

function StageLines.Cleanup()
	Obj.CleanTarget (StageLines.Left)
	Obj.CleanTarget (StageLines.Right)
end
