-- Structure constants
IntroDuration = 0
ExitDuration = 0

Keys = {}

function Init()
	font = Fonts.BitmapFont()
	Fonts.LoadBitmapFont(font, "font.tga", 8, 16, 6, 15, 0)
	display = StringObject2D()
	display.X = 0
	display.Y = 0
	display.Font = font
	display.ScaleX = 2
	display.ScaleY = 2
	Engine:AddTarget(display)
end

--[[ 
--key: scan code
--code: 1 is press, 2 is release (0 was repeat)
--is_mouse_input: 0 if not mouse input, 1 if it is.
--]]
function KeyEvent(key, code, is_mouse_input)
	if code == 1 then
		Keys[key] = code
	else
		Keys[key] = nil
	end
end

-- Intro
function OnIntroBegin()

end

-- Fraction = Time / IntroDuration
function UpdateIntro(Fraction, Delta)

end

function OnIntroEnd()

end

-- Main loop
function Update(Delta)
	local text = "key input test. input code appended below.\n"
	for key, value in pairs(Keys) do
		if value == 1 then
			text = text .. "keycode: " .. key .. "\n"
		end
	end
	
	display.Text = text
end

-- Exit/Outro
function OnExitBegin()

end

function UpdateExit(Fraction, Delta)

end

function OnExitEnd()

end

-- Resource cleanup (i.e. custom allocated resources...)
function Cleanup()

end
