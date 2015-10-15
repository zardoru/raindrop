

Lifebar = {
	FillSize = 500,
	MarginFile = "VSRG/stage-lifeb.png",
	FillFile = "VSRG/stage-lifeb-s.png",
	Width = 50,
	FillWidth = 50,
	Speed = 10,
	FillOffset = -2
}

function Lifebar.Init()	
	Lifebar.Margin = Engine:CreateObject()
	Lifebar.Fill = Engine:CreateObject()
	Lifebar.Fill2 = Engine:CreateObject()

	Lifebar.Margin.Image = (Lifebar.MarginFile)
	Lifebar.Margin.Layer = 25
	Lifebar.Margin.Centered = 1
	-- Lifebar.Margin.Width = Lifebar.Width

	local w = Lifebar.Margin.Width
	local h = Lifebar.Margin.Height
	

	Lifebar.Position = { 
		x = GearStartX + GearWidth + Lifebar.Width / 2 + 8,
		y = ScreenHeight - h / 2
	}

	Lifebar.CurrentPosition = {
		x = Lifebar.Position.x,
		y = ScreenHeight
	}

	Lifebar.Margin.X = Lifebar.CurrentPosition.x
	Lifebar.Margin.Y = Lifebar.Position.y

	Lifebar.Fill.Image = Lifebar.FillFile
	Lifebar.Fill.Width = Lifebar.FillWidth
	Lifebar.Fill.Height = Lifebar.FillSize
	Lifebar.Fill.Layer = 26
	Lifebar.Fill.Centered = 1

	Lifebar.Fill2.Image = Lifebar.FillFile
	Lifebar.Fill2.Layer = 26
	Lifebar.Fill2.Centered = 1
	Lifebar.Fill2.BlendMode = BlendAdd
	Lifebar.Fill2.Width = Lifebar.Fill.Width
	Lifebar.Display = 0
end

function Lifebar.Cleanup()
end

function Lifebar.Run(Delta)
	local DeltaLifebar = (LifebarValue - Lifebar.Display)
	local DP = 1 - (Beat - math.floor(Beat))

	Lifebar.Display = DeltaLifebar * Delta * Lifebar.Speed + Lifebar.Display

	local partA = Lifebar.Display * 0.98
	local partB = Lifebar.Display * 0.02 * DP
	local Display = partA + partB
	local NewY = ScreenHeight - Lifebar.FillSize * (Display) / 2
	local NewYFixed = ScreenHeight - Lifebar.FillSize * (Lifebar.Display) / 2

	Lifebar.CurrentPosition = Lifebar.Position

	Lifebar.Fill.ScaleY = Lifebar.Display 
	Lifebar.Fill.X = Lifebar.Position.x + Lifebar.FillOffset
	Lifebar.Fill.Y = NewYFixed
	Lifebar.Fill:SetCropByPixels( 0, Lifebar.Width, Lifebar.FillSize - Lifebar.FillSize * Lifebar.Display, Lifebar.FillSize )

	Lifebar.Fill2.ScaleY = Display 
	Lifebar.Fill2.X = Lifebar.Position.x + Lifebar.FillOffset
	Lifebar.Fill2.Y = NewY
	Lifebar.Fill2:SetCropByPixels( 0, Lifebar.Width, Lifebar.FillSize - Lifebar.FillSize * Display, Lifebar.FillSize )
	Lifebar.Fill2.Alpha = ( DP * LifebarValue )
end


skin_require "VSRG/judgment"