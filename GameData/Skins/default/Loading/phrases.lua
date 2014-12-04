Phrases = {
	Content = {
		"That guy is saltier than the dead sea.",
		"ayy lmao",
		"nice meme",
		"S P A C E  T I M E",
		"You gonna finish that ice cream sandwich?",
		"TWO DEE ECKS GOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOLD",
		"dude nice",
		"You know what it is, bitch.",
		"You're a master of karate and friendship for everyone!",
		"CHAMPION OF THE ssssssssssssssssssssSUN",
		"what a dumb ass nightingale",
		"C H A O S M A I D  G U Y",
		"What the hell is that.",
		"I'm not good enough for Blocko.",
		"Evasiva coches.",
		"future metallic can - premium skin",
		"2/10",
		"\"what the fuck is VOS\"",
		"Party like it's BM98.",
		"Everyone seems a bit too obsessed with the moon. I wonder if they're werewolves...",
		"thanks mr. skeltal",
		"rice and noodles erryday",
		"reticulating splines",
		":^)",
		"hi spy",
		"hi arcwin",
		"protip: to be overjoy you just have to avoid missing",
		"have you visited theaquila today?",
		"Find us at http://vsrg.club !",
		"\"Eating children is part of our lives.\"",
		"Don't you put it in your mouth.",
		"\"Like the game you may all know stepmania\"",
		"Time to enjoy. I guess.",
		"Are you kaiden yet?",
		"Overmapping is a mapping style.",
		"\"I play volumax.\" - Hazelnut-",
		"\"mario paint music!!\" - peppy",
		"very spammy",
		"your favourite chart is shit",
		"1.33 -> 0.33 -> 1.0 <=/=> 1.5 -> 0.5 -> 1.0",
		"rip words",
		"misses are bad",
		"aiae lmao",
		"\"573 or nothing\"",
		"wats ur favrit 2hu",
		"canmusic makes you ET",
		"youdo me and ideu- you",
		"As easy as ABCD.",
		"You'll full combo it this time.",
		"You're gonna carry that weight.",
		"fappables/duck.gif",
		"16 hours of B.O. blocko power!"
	}
}

function Phrases.Init()
	PhraseFont = Fonts.TruetypeFont(Obj.GetSkinFile("font.ttf"), 30);
	Phrases.Text = StringObject2D()
	Phrases.Text.Font = PhraseFont
	
	local selected = Phrases.Content[math.random(#Phrases.Content)]
	print (selected)
	Phrases.Text.Text = selected
	Phrases.Text.Layer = 14
	Phrases.Text.Alpha = 0
	Phrases.Text.Rotation = 90
	Phrases.Text.X = 80
	
	Engine:AddTarget(Phrases.Text)
end

function Phrases.Fade(frac)
	Phrases.Text.Alpha = frac
end