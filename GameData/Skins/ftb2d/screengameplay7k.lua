game_require("textureatlas.lua")
game_require("frame_interpolator.lua")
game_require("fixed_objects.lua")
game_require("noteskin_defs.lua")
game_require("utils.lua")

skin_require("Global/AnimationFunctions.lua")
skin_require("Global/FadeInScreen.lua")
-- Set up constants for everyone

GearWidth = 275
GearHeight = GearHeightCommon

-- All of these will be loaded in the loading screen instead of
-- in the main thread once loading is over.
Preload = {
	"assets/explosion.png",
	"assets/field.png",
	"assets/glow.png",
	"assets/judge.png",
	"assets/layout.png",
	"assets/belowbelowfield.png",
	"assets/lightning.png",
	"assets/field_limit.png",
	"assets/hp_l.png",
	"assets/hp_r.png",
	"assets/hp_fill.png"
}

JudgmentAtlas = TextureAtlas:new(GetSkinFile("assets/judge.csv"))
ExplosionColorAtlas = TextureAtlas:new(GetSkinFile("assets/colexplosion.csv"))
KeyArray = {}
KeyHold = {}
Judgments = {}
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
		obj.Image = "assets/keyhold.png"
		obj.Height = 60
		obj.Width = 40
		obj.Centered = 1
		obj.X = Noteskin[7]["Key" .. i .. "X"]
		obj.Y = JudgmentLineY + obj.Height / 2 + 5
		obj.Layer = 16
		obj.Alpha = 1
		
		Key[i] = Engine:CreateObject()
		obj = Key[i]
		obj.Image = "Global/white.png"
		obj.Height = 60
		obj.Width = 40
		obj.Centered = 1
		obj.X = Noteskin[7]["Key" .. i .. "X"]
		obj.Y = JudgmentLineY + obj.Height / 2 + 5
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
	ScreenFade.Init()
	ScreenFade.Out(true)

	CreateText()
	
	Glow = FrameInterpolator:New("assets/glow.csv")
	
	Glow.TotalFrames = 35
	Glow.Object.X = GearStartX
	Glow.Object.Y = ScreenHeight - GearWidth + 25
	Glow.Object.Height = 20
	Glow.Object.Width = 280
	Glow.Object.Layer = 20
	
	HealthBar = Engine:CreateObject()
	HealthBar.X = 40
	HealthBar.Y = 600
	HealthBar.Image = "assets/hp_fill.png"
	HealthBar.Width = 280
	HealthBar.Height = 60
	HealthBar.Layer = 19
	HealthBar.Lighten = 1
	HealthBar.LightenFactor = 2
	HealthBar.ScaleX = 0
	
	SongPosition = Engine:CreateObject()
	SongPosition.Image = "assets/songpercent.png"
	SongPosition.Height = 768
	SongPosition.Width = 3
	SongPosition.X = 37
	SongPosition.Y = ScreenHeight
	SongPosition.Layer = 21
	
	Flair = Engine:CreateObject()
	Flair.Image = "assets/flair.png"
	Flair.Height = 768
	Flair.Width = 3
	Flair.X = 320
	Flair.Y = ScreenHeight
	Flair.Layer = 22
	
	Explosions = {}
	Lightning = {}
	for i=1, 7 do
		Explosions[i] = {}
		Explosions[i].EffectExplosion = FrameInterpolator:New("assets/explosion.csv", 0.15)
		
		local obj = Explosions[i].EffectExplosion.Object
		obj.Width = GearWidth / 7
		obj.Height = obj.Width
		obj.X = Noteskin[7]["Key" .. i .. "X"]
		obj.Y = JudgmentLineY
		obj.Centered = 1
		obj.Layer = 20
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
		
		KeyArray[i] = 0
		
		MakeKeys(i)
		
		Judgments[i] = Engine:CreateObject()
		obj = Judgments[i]
		obj.Image = JudgmentAtlas.File
		obj.Width = GearWidth / 7
		obj.Height = 0.234 * obj.Width
		obj.Layer = 22
		obj.Alpha = 0
		obj.Centered = 1
		obj:SetScale(1)
		obj.X = Noteskin[7]["Key" .. i .. "X"]
		obj.Y = JudgmentLineY - obj.Height / 2
	end
	
	
	
	FixedObjects.CreateFromCSV("ftb.csv")
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
	targ.Alpha = 1 - frac
	targ:SetScale(1 + frac * 2.5)
	return 1
end

function Judgment(frac, targ)
	targ.Alpha = 1 - frac
	targ.Y = JudgmentLineY - 100 * frac
	return 1
end

map = {5, 4, 3, 2, 1}
function JUDGE(JudgmentValue, Lane)
	if JudgmentValue == 0 then 
		JudgmentValue = 1
	end
	
	JudgmentAtlas:SetObjectCrop(Judgments[Lane], map[JudgmentValue] .. ".png")
	Engine:AddAnimation(Judgments[Lane], "Judgment", EaseIn, 0.4, 0)
end

function HitEvent(JudgmentValue, TimeOff, Lane, IsHold, IsHoldRelease)
	Engine:AddAnimation(Explosions[Lane].EffectExplosion.Object, "Explode", EaseNone, 0.25, 0)
	Explosions[Lane].EffectExplosion.CurrentTime = 0
	
	JUDGE(JudgmentValue, Lane)
end

function MissEvent(TimeOff, Lane, IsHold)
	JUDGE(5, Lane)
end

function KeyEvent(Key, Code, IsMouseInput)
end

function GearKeyEvent (Lane, IsKeyDown)
	KeyArray[Lane + 1] = IsKeyDown
	
	Lightning[Lane + 1].Object.Alpha = 1
	Lightning[Lane + 1].CurrentTime = 0

	if IsKeyDown == 1 then 
		Key[Lane + 1].LightenFactor = 1
	else
		Key[Lane + 1].LightenFactor = 0
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
	lblCombo.Layer = 14
	
	
	lblmaxCombo = StringObject2D()
	lblmaxCombo.Font = miniFont
	lblmaxCombo.Text = "MAX COMBO"
	lblmaxCombo.X = 230
	lblmaxCombo.Y = 665
	Engine:AddTarget(lblmaxCombo)
	
	lblmaxCombo.Layer = 14
	
	lblcurCombo = StringObject2D()
	lblcurCombo.Font = largeFont
	lblcurCombo.X = 45
	lblcurCombo.Y = 665
	lblcurCombo.Layer = 14
	Engine:AddTarget(lblcurCombo)
	
	lblcurMaxCombo = StringObject2D()
	lblcurMaxCombo.Font = largeFont
	lblcurMaxCombo.Text = "0"
	lblcurMaxCombo.Y = 665
	lblcurMaxCombo.Layer = 14
	Engine:AddTarget(lblcurMaxCombo)
	
	lblScore = StringObject2D()
	lblScore.Font = xlFont
	lblScore.Y = 680
	lblScore.X = 45
	lblScore.Layer = 14
	Engine:AddTarget(lblScore)
	
	lblstScore = StringObject2D()
	lblstScore.Font = miniFont
	lblstScore.Text = "score"
	lblstScore.Y = 740
	lblstScore.X = 155
	lblstScore.Layer = 14
	
	Engine:AddTarget(lblstScore)
end

function UpdateText()
	lblcurCombo.Text = ScoreKeeper:getScore(ST_COMBO)
	lblcurMaxCombo.Text = ScoreKeeper:getScore(ST_MAX_COMBO)
	lblcurMaxCombo.X = 312 - largeFont:GetLength(lblcurMaxCombo.Text)
	
	lblScore.Text = string.format("%08d", SCScore)
	lblScore.X = 188 - largeFont:GetLength(lblScore.Text)
end

function Update(Delta)
	-- Executed every frame.
	local beatEffect = Beat - math.floor(Beat)
	Glow:SetFraction(beatEffect)
	
	local dLifebar = LifebarValue - HealthBar.ScaleX
	HealthBar.ScaleX = HealthBar.ScaleX + dLifebar * Delta
	HealthBar.Green = LifebarValue
	HealthBar.Blue = LifebarValue
	
	UpdateText()
	
	local SongPercentage = Game:GetSongTime() / SongDuration
	
	SongPosition.Y = ScreenHeight - ScreenHeight * SongPercentage
	SongPosition.ScaleY = SongPercentage
	SongPosition:SetCropByPixels(0, 4, 404 - 404 * SongPercentage, 404)
	
	local Acc = ScoreKeeper:getTotalNotes() / ScoreKeeper:getMaxNotes()
	Flair.Y = ScreenHeight - ScreenHeight * Acc
	Flair.ScaleY = Acc
	Flair:SetCropByPixels(0, 4, 404 - 404 * Acc, 404)
	
	for i=1,7 do 
		Explosions[i].EffectExplosion:Update(Delta)
		
		if KeyArray[i] == 1 then
			Lightning[i].CurrentTime = 0
		end
		
		Lightning[i]:Update(Delta)
	end
end

