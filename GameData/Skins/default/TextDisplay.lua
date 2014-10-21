-- text on window

pacemaker1 = StringObject2D();
pacemaker2 = StringObject2D();

lifebar = StringObject2D();

function DrawTextObjects()

	fnt1 = Fonts.TruetypeFont("GameData/Skins/default/font.ttf", 20);
	fnt2 = Fonts.TruetypeFont("GameData/Skins/default/font.ttf", 40);
	-- fnt = Fonts.BitmapFont()
	-- Fonts.LoadBitmapFont(fnt, "font.tga", 6, 16, 5, 13, 0);

	pacemaker1.Text = "";
	pacemaker1.Font = fnt1;
	pacemaker1.X = 100;
	pacemaker1.Y = 286;

	pacemaker2.Text = "";
	pacemaker2.Font = fnt1;
	pacemaker2.X = 160;
	pacemaker2.Y = 286;

	lifebar.Text = "0";
	lifebar.Font = fnt2;
	lifebar.X = 420;
	lifebar.Y = 340;

	Engine:AddTarget(pacemaker1)
	Engine:AddTarget(pacemaker2)
	Engine:AddTarget(lifebar)

end


function UpdateTextObjects()
	
	if PacemakerText then
		pacemaker1.Text = PacemakerText;
	end

	if PacemakerValue then
		pre_char = "+";
		if PacemakerValue < 0 then
			pre_char = "-";
		end
		pacemaker2.Text = string.format("%s%04d", pre_char, math.abs(PacemakerValue));
	end

	if LifebarValue then
		lifebar.Text = LifebarDisplay;
	end
	
end


