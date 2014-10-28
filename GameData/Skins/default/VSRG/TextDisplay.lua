-- text on window

pacemaker1 = StringObject2D();
pacemaker2 = StringObject2D();

lifebar = StringObject2D();

acc1 = StringObject2D();
acc2 = StringObject2D();

function DrawTextObjects()

	fnt1 = Fonts.TruetypeFont(Obj.GetSkinDirectory() .. "font.ttf", 20);
	fnt2 = Fonts.TruetypeFont(Obj.GetSkinDirectory() .. "font.ttf", 40);
	-- fnt = Fonts.BitmapFont()
	-- Fonts.LoadBitmapFont(fnt, "font.tga", 6, 16, 5, 13, 0);

	pacemaker1.Text = "";
	pacemaker1.Font = fnt1;
	pacemaker1.X = Judgment.Position.x - 65;
	pacemaker1.Y = Judgment.Position.y + 20;

	pacemaker2.Text = "";
	pacemaker2.Font = fnt1;
	pacemaker2.X = Judgment.Position.x - 20;
	pacemaker2.Y = Judgment.Position.y + 20;

	lifebar.Text = "0";
	lifebar.Font = fnt2;
	lifebar.X = Lifebar.Position.x + 30;
	lifebar.Y = 340;

	acc1.Text = "";
	acc1.Font = fnt1;
	acc1.X = Judgment.Position.x - 65;
	acc1.Y = Judgment.Position.y + 20;

	acc2.Text = "";
	acc2.Font = fnt1;
	acc2.X = Judgment.Position.x - 20;
	acc2.Y = Judgment.Position.y + 20;

	-- Engine:AddTarget(acc1);
	-- Engine:AddTarget(acc2);
	Engine:AddTarget(pacemaker1);
	Engine:AddTarget(pacemaker2);
	Engine:AddTarget(lifebar);

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

	if AccText then
		acc1.Text = AccText;
	end

	if AccValue then
		acc2.Text = string.format("%.2f%%", AccValue);
	end

	if LifebarValue then
		lifebar.Text = LifebarDisplay;
	end
	
end


