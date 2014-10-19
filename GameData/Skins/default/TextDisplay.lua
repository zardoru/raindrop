pacemaker1 = StringObject2D();
pacemaker2 = StringObject2D();

function DrawTextObjects()

	fnt = Fonts.TruetypeFont("GameData/Skins/default/font.ttf", 20);
	-- fnt = Fonts.BitmapFont()
	-- Fonts.LoadBitmapFont(fnt, "font.tga", 6, 16, 5, 13, 0);

	pacemaker1.Text = "";
	pacemaker1.Font = fnt;
	pacemaker1.X = 100;
	pacemaker1.Y = 286;

	pacemaker2.Text = "";
	pacemaker2.Font = fnt;
	pacemaker2.X = 160;
	pacemaker2.Y = 286;

	Engine:AddTarget(pacemaker1)
	Engine:AddTarget(pacemaker2)

end


function UpdateTextObjects()
	
	if PacemakerText then
		pacemaker1.Text = PacemakerText;
	end

	pre_char = "+";
	
	if PacemakerValue and PacemakerValue < 0 then
		pre_char = "-";
	end

	if PacemakerValue then
		pacemaker2.Text = string.format("%s%04d", pre_char, math.abs(PacemakerValue));
	end
	
end
