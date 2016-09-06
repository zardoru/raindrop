game_require "librd"
skin_require "Global/Background"

function FadeInA1(frac)
	ScreenFade.Black.Alpha = frac
	return 1
end

function invert(f, frac)
	newf = function (frac)
		return f(1 - frac)
	end

	return newf
end

ScreenFade = { Duration = 0.45 }

function ScreenFade.Init()
	BackgroundAnimation:Init()
	ScreenFade.Black = Engine:CreateObject()
	ScreenFade.Black.Texture = "Global/filter.png"

	with(ScreenFade.Black, {
		Width = ScreenWidth,
		Height = ScreenHeight,
		Alpha = 0,
		Layer = 31
	})
	
	IFadeInA1 = invert(FadeInA1)
	return
	--[["Black1 = Engine:CreateObject()
	Black2 = Engine:CreateObject()
	
	Black1.Image = "Global/filter.png"
	Black2.Image = "Global/filter.png"
	
	Black1.Centered = 1
	Black2.Centered = 1
	
	Black1.X = ScreenWidth/2
	Black2.X = ScreenWidth/2
	
	Black1.Y = ScreenWidth/4
	Black2.Y = ScreenWidth*3/4
	
	Black1.Alpha = 1
	Black2.Alpha = 1
	
	Black1.Width = ScreenWidth
	Black2.Width = ScreenWidth
	
	Black1.Height = ScreenHeight/2
	Black2.Height = ScreenHeight/2
	Black1.Z = 31
	Black2.Z = 31
	
	IFadeInA1 = invert(FadeInA1)
	IFadeInA2 = invert(FadeInA2)
	]]
end

function ScreenFade.In(nobg)
	local Delay = 0
	Engine:StopAnimation(ScreenFade.Black)
	
	if not nobg then
		BackgroundAnimation:In()
		Delay = BackgroundAnimation.Duration
	end
	
	Engine:AddAnimation(Black1, "FadeInA1", EaseNone, ScreenFade.Duration, Delay)
	return --[[ Lines beyond are previous implementation.
	Engine:StopAnimation(Black1)
	Engine:StopAnimation(Black2)
	Engine:AddAnimation(Black1, "FadeInA1", EaseNone, 0.2, 0)
	Engine:AddAnimation(Black2, "FadeInA2", EaseNone, 0.2, 0)
	]]
end

function ScreenFade.Out(nobg)
	Engine:StopAnimation(ScreenFade.Black)
	Engine:AddAnimation(nil, "IFadeInA1", EaseNone, ScreenFade.Duration, 0)
	
	if not nobg then
		BackgroundAnimation:Out()
	end
	
	return --[[
	Engine:StopAnimation(Black1)
	Engine:StopAnimation(Black2)
	Engine:AddAnimation(Black1, "IFadeInA1", EaseNone, 0.2, 0)
	Engine:AddAnimation(Black2, "IFadeInA2", EaseNone, 0.2, 0)
	]]
end
