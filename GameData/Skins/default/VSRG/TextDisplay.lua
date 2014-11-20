-- text on window

pacemaker1 = StringObject2D();
pacemaker2 = StringObject2D();
judgments = StringObject2D();

lifebar = StringObject2D();

acc1 = StringObject2D();
acc2 = StringObject2D();

function DrawTextObjects()

	fnt1 = Fonts.TruetypeFont(Obj.GetSkinFile("font.ttf"), 20);
	fnt2 = Fonts.TruetypeFont(Obj.GetSkinFile("font.ttf"), 40);
	fntB = Fonts.BitmapFont()
	Fonts.LoadBitmapFont(fntB, "font.tga", 8, 16, 6, 15, 0);

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

	judgments.Font = fntB
	judgments.X = Lifebar.Position.x + 30
	judgments.Y = 580

	author = StringObject2D()

	author.Font = fnt1
	author.X = lifebar.X
	author.Y = 380

	sng = toSong7K(Global:GetSelectedSong())
	diff = sng:GetDifficulty(Global.DifficultyIndex)
	author.Text = string.format("\n%s by %s\nChart: %s by %s", sng.Title, sng.Author, diff.Name, diff.Author)

	-- Engine:AddTarget(acc1);
	-- Engine:AddTarget(acc2);
	Engine:AddTarget(author)
	Engine:AddTarget(pacemaker1);
	Engine:AddTarget(pacemaker2);
	Engine:AddTarget(lifebar);
	Engine:AddTarget(judgments)

end


function UpdateTextObjects()
	
	if PacemakerText then
		pacemaker1.Text = PacemakerText;
	end

	if PacemakerValue then
		pre_char = "+";
		if PacemakerValue < 0 then
			pre_char = "-";
			pacemaker2.Red = 1
			pacemaker2.Green = 0
			pacemaker2.Blue = 0
		elseif PacemakerValue > 0 then
			pacemaker2.Red = 0.45
			pacemaker2.Green = 0.45
			pacemaker2.Blue = 1
		else
			pacemaker2.Red = 1
			pacemaker2.Green = 1
			pacemaker2.Blue = 1
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
		lifebar.Text = string.format("%03d%%", LifebarDisplay);
	end

	local mlt = Game:GetUserMultiplier()
	local vspd = Game:GetCurrentVerticalSpeed()
	local fmtext = string.format("Speed: %02.2fx (%.0f -> %.0f)\n", mlt, vspd, mlt*vspd)
	local w0, w1, w2, w3, w4, w5

	w0 = ScoreKeeper:getJudgmentCount(SKJ_W0)
	w1 = ScoreKeeper:getJudgmentCount(SKJ_W1)
	w2 = ScoreKeeper:getJudgmentCount(SKJ_W2)
	w3 = ScoreKeeper:getJudgmentCount(SKJ_W3)
	w4 = ScoreKeeper:getJudgmentCount(SKJ_W4)
	w5 = ScoreKeeper:getJudgmentCount(SKJ_MISS)
	if ScoreKeeper:usesW0() == false then
		if ScoreKeeper:usesO2() == false then
			fmtext = fmtext .. string.format("E:%04d\nS:%04d\nN:%04d\nO:%04d\nM:%04d", w1, w2, w3, w4, w5)
		else
			local p = ScoreKeeper:getPills()
			local rem = 15 - ScoreKeeper:getCoolCombo() % 15
			fmtext = fmtext .. string.format("F:%04d\nE:%04d\nO:%04d\nM:%04d\nP:%04d / CC: %04d", w1, w2, w3, w5, p, rem)
		end
	else
		fmtext = fmtext .. string.format("F:%04d\nE:%04d\nS:%04d\nN:%04d\nO:%04d\nM:%04d", w0, w1, w2, w3, w4, w5)
	end

	fmtext = fmtext .. string.format("\nMaxCombo: %d", ScoreKeeper:getScore(ST_MAX_COMBO))
	fmtext = fmtext .. string.format("\nBPM: %d", CurrentBPM)


	judgments.Text = fmtext
end


