-- text on window

PlayerText = {}

function PlayerText:Init()

	self.pacemaker1 = StringObject2D();
	self.pacemaker2 = StringObject2D();
	self.judgments = StringObject2D();

	self.lifebar = StringObject2D();
	
	if fnt1 == nil then 
		fnt1 = Fonts.TruetypeFont(GetSkinFile("font.ttf"), 20);
		fnt2 = Fonts.TruetypeFont(GetSkinFile("font.ttf"), 40);
		fntB = Fonts.BitmapFont()
		Fonts.LoadBitmapFont(fntB, "font.tga", 8, 16, 6, 15, 0);
	end
	
	-- matches default judgment
	local jX = self.Noteskin.GearWidth / 2 + self.Noteskin.GearStartX
  	local jY = ScreenHeight * 0.4


	self.pacemaker1.Text = "";
	self.pacemaker1.Font = fnt1;
	self.pacemaker1.X = jX - 65;
	self.pacemaker1.Y = jY + 20;
  	self.pacemaker1.Layer = 24;

	self.pacemaker2.Text = "";
	self.pacemaker2.Font = fnt1;
	self.pacemaker2.X = jX - 20;
	self.pacemaker2.Y = jY + 20;
  	self.pacemaker2.Layer = 26;

	self.lifebar.Text = "0";
	self.lifebar.Font = fnt2;
	self.lifebar.X = self.Noteskin.GearStartX + self.Noteskin.GearWidth + 80;
	self.lifebar.Y = 340;

	self.judgments.Font = fntB
	self.judgments.X = self.lifebar.X
	self.judgments.Y = 560

	self.author = StringObject2D()

	self.author.Font = fnt1
	self.author.X = self.lifebar.X
	self.author.Y = 380

	local sng = Global:GetSelectedSong()
	local diff = self.Player.Difficulty
	if diff.Author ~= "" then
		difftxt = string.format("%s by %s", diff.Name, diff.Author)
	else
		difftxt = string.format("%s", diff.Name)
	end
	self.author.Text = string.format("\n%s by %s\n%s", sng.Title, sng.Author, difftxt)

	Engine:AddTarget(self.author)
	Engine:AddTarget(self.pacemaker1);
	Engine:AddTarget(self.pacemaker2);
	Engine:AddTarget(self.lifebar);
	Engine:AddTarget(self.judgments)
end

librd.make_new(PlayerText, PlayerText.Init)

function PlayerText:Run(dt)
	
	--[[
		You get two pacemakers: 
		Raindrop Rank (PacemakerText/PacemakerValue)
		BMS Rank (BMPacemakerText/BMPacemakerValue)
		
		Switch between them as you will.
	]]

	-- true = bm, false = rdr
	local pmt = self.Player:GetPacemakerText(true)
	local pmv = self.Player:GetPacemakerValue(true)
	
	-- The following update the pacemaker for real.
	if pmt then
		self.pacemaker1.Text = pmt;
	end

	if pmv then
		pre_char = "+";
		if pmv < 0 then
			pre_char = "-";
			self.pacemaker2.Red = 1
			self.pacemaker2.Green = 0
			self.pacemaker2.Blue = 0
		elseif pmv > 0 then
			self.pacemaker2.Red = 0.45
			self.pacemaker2.Green = 0.45
			self.pacemaker2.Blue = 1
		else
			self.pacemaker2.Red = 1
			self.pacemaker2.Green = 1
			self.pacemaker2.Blue = 1
		end
		
		self.pacemaker2.Text = string.format("%s%04d", pre_char, math.abs(pmv));
	end

	self.lifebar.Text = string.format("%03d%%", self.Player.LifebarPercent);

	local mlt = self.Player.SpeedMultiplier
	local vspd = self.Player.Speed
	local fmtext = string.format("Speed: %02.2fx (%.0f -> %.0f)\n", mlt, vspd, mlt*vspd)
	local ScoreKeeper = self.Player.Scorekeeper
	local w0, w1, w2, w3, w4, w5

	w0 = ScoreKeeper:GetJudgmentCount(SKJ_W0)
	w1 = ScoreKeeper:GetJudgmentCount(SKJ_W1)
	w2 = ScoreKeeper:GetJudgmentCount(SKJ_W2)
	w3 = ScoreKeeper:GetJudgmentCount(SKJ_W3)
	w4 = ScoreKeeper:GetJudgmentCount(SKJ_W4)
	w5 = ScoreKeeper:GetJudgmentCount(SKJ_MISS)
	if ScoreKeeper.UsesW0 == false then
		if ScoreKeeper.UsesO2 == false then
			fmtext = fmtext .. string.format("E:%04d\nS:%04d\nN:%04d\nO:%04d\nM:%04d", w1, w2, w3, w4, w5)
		else
			local p = ScoreKeeper.Pills
			local rem = 15 - ScoreKeeper.CoolCombo % 15
			fmtext = fmtext .. string.format("F:%04d\nE:%04d\nO:%04d\nM:%04d\nP:%04d / CC: %04d", w1, w2, w3, w5, p, rem)
		end
	else
		fmtext = fmtext .. string.format("F:%04d\nE:%04d\nS:%04d\nN:%04d\nO:%04d\nM:%04d", w0, w1, w2, w3, w4, w5)
	end

	fmtext = fmtext .. string.format("\nMaxCombo: %d", ScoreKeeper:GetScore(ST_MAX_COMBO))
	fmtext = fmtext .. string.format("\nBPM: %d", self.Player.BPM)
  	fmtext = fmtext .. string.format("\nAvg. Hit (ms): %f", ScoreKeeper.AvgHit)

	self.judgments.Text = fmtext
end


