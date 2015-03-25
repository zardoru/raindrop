
Filter = { Image = "Global/filter.png" }
JudgeLine = { Image = "VSRG/judgeline.png" }
StageLines = { ImageLeft = "VSRG/stage-left.png", ImageRight = "VSRG/stage-right.png" }

function Filter.Init()
	if GetConfigF("ScreenFilter", "") == 0 then
		return
	end

	Filter.Object = Engine:CreateObject()

	Filter.Object.Image = (Filter.Image)
	Filter.Object.X = GearStartX
	Filter.Object.Width = GearWidth
	Filter.Object.Height = ScreenHeight
	Filter.Object.Alpha = 1
	Filter.Object.Layer = 1
end


function JudgeLine.Init()
	JudgeLine.Object = Engine:CreateObject()
	JudgeLine.Size = { w = GearWidth, h = GetConfigF("NoteHeight", "") }

	JudgeLine.Object.Image = JudgeLine.Image
	JudgeLine.Object.Centered = 1

	JudgeLine.Object.X = GearStartX + GearWidth / 2
	JudgeLine.Object.Y = JudgmentLineY
	
	JudgeLine.Object.Width = JudgeLine.Size.w
	JudgeLine.Object.Height = JudgeLine.Size.h
	JudgeLine.Object.Layer = 12
end

function StageLines.Init()
	StageLines.Left = Engine:CreateObject()
	StageLines.Right = Engine:CreateObject()
	
	StageLines.Left.Image = (StageLines.ImageLeft)
	StageLines.Left.X = GearStartX - StageLines.Left.Width
	StageLines.Left.Height = ScreenHeight
	StageLines.Left.Layer = 20

	StageLines.Right.Image = (StageLines.ImageRight)
	StageLines.Right.X = (GearStartX + GearWidth)
	StageLines.Right.Height = ScreenHeight
	StageLines.Right.Layer = 20
end
