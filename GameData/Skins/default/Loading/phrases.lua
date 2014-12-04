Phrases = {
	Content = {
		"That guy is saltier than the dead sea.",
		"not even kaiden yet",
		"a noodle soup a day and your skills won't decay",
		"hey girl im kaiden want to go out",
		"now with more drops, sadly rain does not produce dubstep.",
		"i dropped out of school to play music games",
		"tropical storm more like tropical fart",
		"protip: dolphins are not capable of playing music games, let alone make music for them.",
		"did you hear about this cool game called beatmani?",
		"to be honest, it's not ez to dj.",
		"at least we won't lose our source code.",
		"less woosh more drop",
		"studies show that certain rhythm game communities contain more cancerous and autistic people than other communities.",
		"hot new bonefir remix knife party",
		"i'll only date you if you're kaiden",
		"it's called overjoy because the people who plays those charts are masochists",
		"studies show that combo-based scoring is the biggest factor of broken equipment in the rhythm game community",
		"YOU GET 200 GOLD CROWNS! IT IS EXTRAORDINARY!! YOU ARE THE TRUE TATSUJIN",
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
		"16 hours of B.O. blocko power!",
		"how can there be 714 bpm if theres only 60 seconds in a minute?",
		"Far East Nightbird (Twitch remix)",
		"Just hold on. You'll be fine, I promise. Everyday.",
		"2spooky"
	}
}

function Phrases.Init()
	Phrases.VSize = 22
	PhraseFont = Fonts.TruetypeFont(Obj.GetSkinFile("font.ttf"), Phrases.VSize);
	Phrases.Text = StringObject2D()
	Phrases.Text.Font = PhraseFont
	
	local selected = Phrases.Content[math.random(#Phrases.Content)]
	Phrases.Text.Text = selected
	Phrases.Text.Layer = 12
	Phrases.Text.Alpha = 0
	Phrases.Text.Rotation = 0
	Phrases.Text.X = 0
	Phrases.Text.Y = -Phrases.VSize
	
	Phrases.BG = Object2D()
	Phrases.BG.Image = "Global/filter.png"
	Phrases.BG.Height = 30
	Phrases.BG.Layer = 11
	Phrases.BG.Width = ScreenWidth
	Phrases.BG.X = 0
	
	Engine:AddTarget(Phrases.BG)
	Engine:AddTarget(Phrases.Text)
end

function Phrases.Fade(frac)
	Phrases.Text.Alpha = frac
	Phrases.Text.Y = -Phrases.VSize + Phrases.VSize * frac
	
	Phrases.BG.Alpha = frac
	Phrases.BG.Y = -Phrases.BG.Height + Phrases.BG.Height * frac
end