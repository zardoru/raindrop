

Lifebar = {
	FillSize = 500,
	MarginFile = "VSRG/stage-lifeb.png",
	FillFile = "VSRG/stage-lifeb-s.png",
	Width = 50,
	FillWidth = 50,
	Speed = 10,
	FillOffset = -2
}

Jambar = {
	ImageFG = "VSRG/jam_bar.png"
}

function Lifebar:Init()	
	self.Margin = Engine:CreateObject()
	self.Fill = Engine:CreateObject()
	self.Fill2 = Engine:CreateObject()

	self.Margin.Image = self.MarginFile
	self.Margin.Layer = 25
	self.Margin.Centered = 1
	-- self.Margin.Width = self.Width

	local w = self.Margin.Width
	local h = self.Margin.Height
	

	self.Position = { 
		x = self.Noteskin.GearStartX + self.Noteskin.GearWidth + self.Width / 2 + 8,
		y = ScreenHeight - h / 2
	}

	self.CurrentPosition = {
		x = self.Position.x,
		y = ScreenHeight
	}

	self.Margin.X = self.CurrentPosition.x
	self.Margin.Y = self.Position.y

	self.Fill.Image = self.FillFile
	self.Fill.Width = self.FillWidth
	self.Fill.Height = self.FillSize
	self.Fill.Layer = 26
	self.Fill.Centered = 1

	self.Fill2.Image = self.FillFile
	self.Fill2.Layer = 26
	self.Fill2.Centered = 1
	self.Fill2.BlendMode = BlendAdd
	self.Fill2.Width = self.Fill.Width
	self.Display = 0
end

librd.make_new(Lifebar, Lifebar.Init)

function Lifebar:Run(Delta)
	local DeltaLifebar = (LifebarValue - self.Display)
	local DP = 1 - fract(self.Player.Beat)

	self.Display = DeltaLifebar * Delta * self.Speed + self.Display

	local partA = self.Display * 0.98
	local partB = self.Display * 0.02 * DP
	local Display = partA + partB
	local NewY = ScreenHeight - self.FillSize * (Display) / 2
	local NewYFixed = ScreenHeight - self.FillSize * (self.Display) / 2

	self.CurrentPosition = self.Position

	self.Fill.ScaleY = self.Display 
	self.Fill.X = self.Position.x + self.FillOffset
	self.Fill.Y = NewYFixed
	self.Fill:SetCropByPixels( 0, self.Width, self.FillSize - self.FillSize * self.Display, self.FillSize )

	self.Fill2.ScaleY = Display 
	self.Fill2.X = self.Position.x + self.FillOffset
	self.Fill2.Y = NewY
	self.Fill2:SetCropByPixels( 0, self.Width, self.FillSize - self.FillSize * Display, self.FillSize )
	self.Fill2.Alpha = ( DP * LifebarValue )
end


function Jambar:Init()
  self.Width = self.Margin.Width
  self.Height = self.Height or 335

	self.BarFG = Engine:CreateObject()
	self.BarFG.Image = self.ImageFG;
  
	with (self.BarFG, {
		Centered = 1,
		X = self.Margin.X,
	  Layer = self.Margin.Layer,
		Width = Jambar.Width,
	  Height = Jambar.Height,
	  Lighten = 1
	})
end

librd.make_new(Jambar, Jambar.Init)

function Jambar:Run(Delta)
  local targetRem
  local ScoreKeeper = self.Player.ScoreKeeper
  
  if ScoreKeeper.UsesO2 == false then
    targetRem = ScoreKeeper.JudgedNotes / ScoreKeeper.MaxNotes
  else
    targetRem = 1 - (15 - (ScoreKeeper.CoolCombo % 15 + 1)) / 15
  end

  -- Percentage from 0 to 1 of cool combo

  self.BarFG.LightenFactor = 1 - fract(self.Player.Beat)

  local Offset = targetRem * Jambar.Height
  self.BarFG.ScaleY = targetRem
  self.BarFG.Y = ScreenHeight - Offset / 2
  self.BarFG:SetCropByPixels( 0, self.Width, self.BarFG.Height - Offset, self.BarFG.Height )
end