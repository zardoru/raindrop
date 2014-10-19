function DrawTextObjects()

	fnt = Fonts.TruetypeFont("GameData/Skins/default/font.ttf", 20);
	-- fnt = Fonts.BitmapFont()
	-- Fonts.LoadBitmapFont(fnt, "font.tga", 6, 16, 5, 13, 0);

	x = StringObject2D();
	x.Text = "TEXT TEXT TEXT";
	x.Font = fnt;
	x.X = 168;
	x.Y = 240;

	Engine:AddTarget(x)

end
