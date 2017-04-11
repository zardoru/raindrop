if Game:GetPlayer(0).Channels > 7 then
	fallback_require("screengameplay7k")
	return
end

game_require("TextureAtlas")
game_require("FrameInterpolator")
game_require("FixedObjects")
skin_require("custom_defs")
game_require("utils")
game_require("AnimationFunctions")

skin_require("Global/FadeInScreen")
-- Set up constants for everyone

GearWidth = 275
GearHeight = GearHeightCommon

-- All of these will be loaded in the loading screen instead of
-- in the main thread, and will also be unloaded at the end.
Preload = {
	"assets/explosion.png",
	"assets/field.png",
	"assets/glow.png",
	"assets/judge.png",
	"assets/belowbelowfield.png",
	"assets/lightning.png",
	"assets/field_limit.png",
	"assets/hp_l.png",
	"assets/hp_r.png",
	"assets/hp_fill.png",
	"assets/notes.png",
	"assets/holds.png",
	"assets/flair.png",
	"assets/hp_bottom.png",
	"assets/keyhold.png",
	"assets/songpercent.png"
}

JudgmentAtlas = TextureAtlas:new(GetSkinFile("assets/judge.csv"))
ExplosionColorAtlas = TextureAtlas:new(GetSkinFile("assets/colexplosion.csv"))

-- Status of a lane being pressed or not.
KeyArray = {}

-- Key overlay.
KeyHold = {}

-- Judgments.
Judgments = {}

-- The keys themselves.
Key = {}

Colours = {
	{255, 0, 204},
	{255, 204, 0},
	{203, 255, 0},
	{0, 203, 255}
}
Layout = {1, 2, 3, 4, 3, 2, 1}

function MakeKeys(i)
		KeyHold[i] = Engine:CreateObject()
		obj = KeyHold[i]
		obj.Texture = "assets/keyhold.png"
		obj.Height = 60
		obj.Width = 40
		obj.Centered = 1
		obj.X = Noteskin[7]["Key" .. i .. "X"]
		obj.Y = Game:GetPlayer(0).JudgmentY + obj.Height / 2 + 5
		obj.Layer = 16
		obj.Alpha = 1
		
		Key[i] = Engine:CreateObject()
		obj = Key[i]
		obj.Texture = "Global/white.png"
		obj.Height = 60
		obj.Width = 40
		obj.Centered = 1
		obj.X = Noteskin[7]["Key" .. i .. "X"]
		obj.Y = Game:GetPlayer(0).JudgmentY + obj.Height / 2 + 5
		obj.Layer = 16
		obj.Alpha = 1
		obj.Lighten = 1
		obj.LightenFactor = 0
		
		obj.Red = Colours[Layout[i]][1] / 255
		obj.Green = Colours[Layout[i]][2] / 255
		obj.Blue = Colours[Layout[i]][3] / 255
		
end

function Init()
	AutoadjustBackground()
	
	CreateText()
	
	print "Creating animated objects"
	Glow = FrameInterpolator:New("assets/glow.csv")
	
	Glow.TotalFrames = 35
	Glow.Object.X = GearStartX
	Glow.Object.Y = ScreenHeight - GearWidth + 25
	Glow.Object.Height = 20
	Glow.Object.Width = 280
	Glow.Object.Layer = 20
	
	print "Creating fixed objects."
	GameObjects = FixedObjects:new()
	GameObjects:CreateFromCSV("ftb.csv")
	
	Sprites = GameObjects.Sprites
	HealthBar = Sprites['hp_fill']
	HealthBar.Lighten = 1
	HealthBar.LightenFactor = 2
	HealthBar.ScaleX = 0
	
	SongPosition = Sprites["songpercent"]
	
	Flair = Sprites["flair"]
	
	print "Creating explosions and lightning."
	Explosions = {}
	Lightning = {}
	for i=1, 7 do
		Explosions[i] = {}
		Explosions[i].EffectExplosion = FrameInterpolator:New("assets/explosion.csv", 0.15)
		
		local obj = Explosions[i].EffectExplosion.Object
		obj.Width = GearWidth / 7
		obj.Height = obj.Width
		obj.X = Noteskin[7]["Key" .. i .. "X"]
		obj.Y = Game:GetPlayer(0).JudgmentY
		obj.Centered = 1
		obj.Layer = 21
		obj.Alpha = 0
		
		obj.Red = Colours[Layout[i]][1] / 255
		obj.Green = Colours[Layout[i]][2] / 255
		obj.Blue = Colours[Layout[i]][3] / 255
		
		Lightning[i] = FrameInterpolator:New("assets/lightning.csv", 0.4)
		obj = Lightning[i].Object
		obj.Height = 540
		obj.Width = GearWidth / 7
		obj.X = Noteskin[7]["Key" .. i .. "X"] - obj.Width / 2
		obj.Y = 0
		obj.Layer = 24
		obj.Alpha = 0
		
		obj.Red = Colours[Layout[i]][1] / 255
		obj.Green = Colours[Layout[i]][2] / 255
		obj.Blue = Colours[Layout[i]][3] / 255
		
		KeyArray[i] = false
		
		MakeKeys(i)
		
		Judgments[i] = Engine:CreateObject()
		obj = Judgments[i]
		obj.Texture = JudgmentAtlas.File
		obj.Width = GearWidth / 7
		obj.Height = 0.234 * obj.Width
		obj.Layer = 22
		obj.Alpha = 0
		obj.Centered = 1
		obj:SetScale(1)
		obj.X = Noteskin[7]["Key" .. i .. "X"]
		obj.Y = Game:GetPlayer(0).JudgmentY - obj.Height / 2
	end
	
	Engine:Sort()
end

function Cleanup()
end

function OnFullComboEvent()
end

function OnFailureEvent()
	return 0
end

-- When 'enter' is pressed and the game starts, this function is called.
function OnActivateEvent()
end

function Explode(frac, targ)
	targ:SetScale(1 + frac * 2.5)
  targ.Alpha = 1 - frac
	return 1
end

function Judgment(frac, targ)
	targ.Alpha = 1 - frac
	targ.Y = Game:GetPlayer(0).JudgmentY - 100 * frac
	return 1
end

-- The judgments are in inverted order, so yup.
map = {5, 4, 3, 2, 1}
function JUDGE(JudgmentValue, Lane)
	if JudgmentValue == 0 then 
		JudgmentValue = 1
	end
	
	JudgmentAtlas:SetObjectCrop(Judgments[Lane], map[JudgmentValue] .. ".png")
	Engine:AddAnimation(Judgments[Lane], "Judgment", EaseIn, 0.4, 0)
end

function HitEvent(JudgmentValue, TimeOff, Lane, IsHold, IsHoldRelease)
  local MapLane = Noteskin[Game:GetPlayer(0).Channels].Map[Lane]
  
	Engine:AddAnimation(Explosions[MapLane].EffectExplosion.Object, "Explode", EaseNone, 0.25, 0)
	Explosions[MapLane].EffectExplosion.CurrentTime = 0
	
	JUDGE(JudgmentValue, MapLane)
end

function MissEvent(TimeOff, Lane, IsHold)
	JUDGE(5, Lane)
end

function KeyEvent(Key, Code, IsMouseInput)
end

function GearKeyEvent (Lane, IsKeyDown)
  local MapLane = Noteskin[Game:GetPlayer(0).Channels].Map[Lane]
  
	KeyArray[MapLane] = IsKeyDown
	
	Lightning[MapLane].Object.Alpha = 1
	Lightning[MapLane].CurrentTime = 0

	if IsKeyDown == 1 then 
		Key[MapLane].LightenFactor = 1
	else
		Key[MapLane].LightenFactor = 0
	end
end

-- Called when the song is over.
function OnSongFinishedEvent()
	return 0
end

function CreateText()
	miniFont = Fonts.TruetypeFont(GetSkinFile("ftb_font.ttf"), 16)
	largeFont = Fonts.TruetypeFont(GetSkinFile("ftb_font.ttf"), 36)
	xlFont = Fonts.TruetypeFont(GetSkinFile("ftb_font.ttf"), 61)
	
	print "Creating text."
	lblCombo = StringObject2D()
	lblCombo.Font = miniFont
	lblCombo.Text = "Combo"
	lblCombo.X = 45
	lblCombo.Y = 665
	Engine:AddTarget(lblCombo)
	lblCombo.Layer = 24
	
	
	lblmaxCombo = StringObject2D()
	lblmaxCombo.Font = miniFont
	lblmaxCombo.Text = "MAX COMBO"
	lblmaxCombo.X = 230
	lblmaxCombo.Y = 665
	Engine:AddTarget(lblmaxCombo)
	
	lblmaxCombo.Layer = 24
	
	lblcurCombo = StringObject2D()
	lblcurCombo.Font = largeFont
	lblcurCombo.X = 45
	lblcurCombo.Y = 665
	lblcurCombo.Layer = 24
	Engine:AddTarget(lblcurCombo)
	
	lblcurMaxCombo = StringObject2D()
	lblcurMaxCombo.Font = largeFont
	lblcurMaxCombo.Text = "0"
	lblcurMaxCombo.Y = 665
	lblcurMaxCombo.Layer = 24
	Engine:AddTarget(lblcurMaxCombo)
	
	lblScore = StringObject2D()
	lblScore.Font = xlFont
	lblScore.Y = 680
	lblScore.X = 45
	lblScore.Layer = 24
	Engine:AddTarget(lblScore)
	
	lblstScore = StringObject2D()
	lblstScore.Font = miniFont
	lblstScore.Text = "score"
	lblstScore.Y = 740
	lblstScore.X = 155
	lblstScore.Layer = 24
	
	Engine:AddTarget(lblstScore)
end

function UpdateText()
	local ScoreKeeper = Game:GetPlayer(0).Scorekeeper
	lblcurCombo.Text = ScoreKeeper:GetScore(ST_COMBO)
	lblcurMaxCombo.Text = ScoreKeeper:GetScore(ST_MAX_COMBO)
	lblcurMaxCombo.X = 312 - largeFont:GetLength(lblcurMaxCombo.Text)
	
	lblScore.Text = string.format("%08d", Game:GetPlayer(0).Score)
	local len = xlFont:GetLength(lblScore.Text)
	lblScore.X = 42 + math.abs(GearWidth - len) / 2
	
end

function Update(Delta)
	local Beat = Game:GetPlayer(0).Beat
	-- Executed every frame.
	local beatEffect = Beat - math.floor(Beat)
	Glow:SetFraction(beatEffect)
	
	local dLifebar = Game:GetPlayer(0).LifebarPercent / 100 - HealthBar.ScaleX
	HealthBar.ScaleX = HealthBar.ScaleX + dLifebar * Delta
	HealthBar.Green = Game:GetPlayer(0).LifebarPercent / 100
	HealthBar.Blue = Game:GetPlayer(0).LifebarPercent / 100
	
	UpdateText()
	
	local SongPercentage = Game:GetPlayer(0).Time / Game:GetPlayer(0).Duration
	
	SongPosition.Y = ScreenHeight - ScreenHeight * SongPercentage
	SongPosition.ScaleY = SongPercentage
	SongPosition:SetCropByPixels(0, 4, 404 - 404 * SongPercentage, 404)
	
	local ScoreKeeper = Game:GetPlayer(0).Scorekeeper
	local Acc = ScoreKeeper.JudgedNotes / ScoreKeeper.MaxNotes
	Flair.Y = ScreenHeight - ScreenHeight * Acc
	Flair.ScaleY = Acc
	Flair:SetCropByPixels(0, 4, 404 - 404 * Acc, 404)
	
	for i=1,7 do 
		Explosions[i].EffectExplosion:Update(Delta)
		
		if KeyArray[i] then
			Lightning[i].CurrentTime = 0
		end
		
		Lightning[i]:Update(Delta)
	end
end

