
Filter = { Image = "Global/Filter.png" }
JudgeLine = { Image = "VSRG/JudgeLine.png" }
StageLines = { ImageLeft = "VSRG/stage-left.png", ImageRight = "VSRG/stage-right.png" }

function Filter:Init()
	if GetConfigF("ScreenFilter", "") == 0 then
		return
	end
	
	FilterVal = GetConfigF("ScreenFilter", "")

	self.Object = Engine:CreateObject()

	self.Object.Texture = (self.Image)
  
	self.Object.X = self.Noteskin.GearStartX
	self.Object.Width = self.Noteskin.GearWidth
	self.Object.Height = ScreenHeight
	self.Object.Alpha = FilterVal
	self.Object.Layer = 1
end

librd.make_new(Filter, Filter.Init)

function JudgeLine:Init()
	self.Object = Engine:CreateObject()
	self.Size = { w = self.Noteskin.GearWidth, h = self.Noteskin.NoteHeight }

	self.Object.Texture = self.Image
	self.Object.Centered = 1

	self.Object.X = self.Noteskin.GearStartX + self.Noteskin.GearWidth / 2
	self.Object.Y = self.Player.JudgmentY
	
	self.Object.Width = self.Size.w
	self.Object.Height = self.Size.h
	self.Object.Layer = 12
end

librd.make_new(JudgeLine, JudgeLine.Init)

function StageLines:Init()
	self.Left = Engine:CreateObject()
	self.Right = Engine:CreateObject()
	
	self.Left.Texture = self.ImageLeft
	self.Left.X = self.Noteskin.GearStartX - self.Left.Width
	self.Left.Height = ScreenHeight
	self.Left.Layer = 16

	self.Right.Texture = (self.ImageRight)
	self.Right.X = (self.Noteskin.GearStartX + self.Noteskin.GearWidth)
	self.Right.Height = ScreenHeight
	self.Right.Layer = 20
end

librd.make_new(StageLines, StageLines.Init)