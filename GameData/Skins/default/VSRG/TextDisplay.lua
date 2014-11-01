-- text on window

pacemaker1 = StringObject2D();
pacemaker2 = StringObject2D();
judgments = StringObject2D();

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

	judgments.Font = fnt1
	judgments.X = Lifebar.Position.x + 30
	judgments.Y = 380

	-- Engine:AddTarget(acc1);
	-- Engine:AddTarget(acc2);
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

	local fmtext= string.format("Speed: %02.2fx\n", Game:GetUserMultiplier())
	local w0, w1, w2, w3, w4, w5

	w0 = ScoreKeeper:getJudgmentCount(SKJ_W0)
	w1 = ScoreKeeper:getJudgmentCount(SKJ_W1)
	w2 = ScoreKeeper:getJudgmentCount(SKJ_W2)
	w3 = ScoreKeeper:getJudgmentCount(SKJ_W3)
	w4 = ScoreKeeper:getJudgmentCount(SKJ_W4)
	w5 = ScoreKeeper:getJudgmentCount(SKJ_W5)
	if ScoreKeeper:usesW0() ~= 0 then
		fmtext = fmtext .. string.format("%04d\n%04d\n%04d\n%04d\n%04d", w1, w2, w3, w4, w5)
	else
		fmtext = fmtext .. string.format("%04d\n%04d\n%04d\n%04d\n%04d\n%04d", w0, w1, w2, w3, w4, w5)
	end

	judgments.Text = fmtext
end


