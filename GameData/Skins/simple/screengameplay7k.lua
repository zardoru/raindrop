if Game:GetPlayer(0).Channels ~= 8 then
	fallback_require "screengameplay7k"
	return
end

game_require "TextureAtlas"
game_require "FixedObjects"

skin_require "custom_defs"

game_require "utils"
game_require "AnimationFunctions"

-- Set up constants for everyone


-- Scale from 1280x720
YR = 768 / 720
XR = 1360 / 1280

GearWidth = 380
GearHeight = GearHeightCommon

skin_require "Scripts/ComboDisplay"
skin_require "Scripts/Judgment"
skin_require "Scripts/ScoreDisplay"

Judgment.Scale = 0.15
Judgment.ScaleHit = 1.2
Judgment.ScaleMiss = 0.1
Judgment.Position = {
	y = 400,
	x = GearStartX + Noteskin[8].GearWidth / 2 - 50
}

Judgment.Tilt = 0
ComboDisplay.DigitWidth = 25
ComboDisplay.DigitHeight = 25
ComboDisplay.Position = {
	x = Judgment.Position.x + 200,
	y = Judgment.Position.y
}
ComboDisplay.BumpVertically = 0
ComboDisplay.BumpHorizontally = 0
ComboDisplay.HeightAddition = 0
ComboDisplay.HoldBumpFactor = 1

ScoreDisplay.X = 500 * XR
ScoreDisplay.Y = 680 * YR
ScoreDisplay.DigitWidth = 30
ScoreDisplay.DigitWidth = 30

-- All of these will be loaded in the loading screen instead of
-- in the main thread, and will also be unloaded at the end.
Preload = {
	"assets/bomb.png",
	"assets/life.png",
	"assets/progress_tick.png",
	"assets/stage.png",
	"assets/tt.png",
}

-- Status of a lane being pressed or not.
KeyArray = {}

-- Key overlay.
KeyHold = {}

-- Judgments.
Judgments = {}

-- The keys themselves.
Key = {}

Lightning = {}
LightingTime = 0.25

Bomb = {}
BombTime = 0.2

function GenText()
	NumFont = Fonts.TruetypeFont(GetSkinFile("VeraMono.ttf"), 45 * YR)
	TitleFont = Fonts.TruetypeFont(GetSkinFile("font.ttf"), 15 * YR)
	BPMText = StringObject2D()
	BPMText.X = 95 * XR
	BPMText.Y = 660 * YR
	BPMText.Font = NumFont
	BPMText.Layer = 21

	HPText = StringObject2D()
	HPText.X = 315 * XR
	HPText.Y = 605 * YR
	HPText.Font = NumFont
	HPText.Layer = 21

	TitleText = StringObject2D()
	TitleText.X = 455 * XR
	TitleText.Y = 5 * YR
	TitleText.Font = TitleFont
	TitleText.Layer = 21

	sng = Global:GetSelectedSong()
	diff = Game:GetPlayer(0).Difficulty
	if diff.Author ~= "" then
		difftxt = string.format("%s by %s", diff.Name, diff.Author)
	else
		difftxt = string.format("%s", diff.Name)
	end
	TitleText.Text = string.format("%s\n%s\n%s", sng.Title, sng.Author, difftxt)

	Engine:AddTarget(BPMText)
	Engine:AddTarget(HPText)
	Engine:AddTarget(TitleText)
end

function Init()
	FixedObjects.XRatio = XR
	FixedObjects.YRatio = YR
	AutoadjustBackground({
		x = 540 * XR,
		y = 84 * YR,
		w = 640 * XR,
		h = 480 * YR
	})


	local tbl = {
		Player = Game:GetPlayer(0),
		Noteskin = Noteskin[8]
	}

	combodisplay = ComboDisplay:new(tbl)
	judgment = Judgment:new(tbl)
	scoredisplay = ScoreDisplay:new(tbl)
	IsFullCombo = false

	Channels = Game:GetPlayer(0).Channels

	GenText()
	print "Creating fixed objects."
	GameObjects = FixedObjects:new()
	GameObjects:CreateFromCSV("simple.csv", Noteskin[8])
	Sprites = GameObjects.Sprites 

	SongPosition = Sprites["tick"]
	SongPosition.Centered = true

	tabletick = Sprites["tt"]
	tabletick.Centered = true

	for i=1, Channels do
		if i < 8 then
			Key[i] = Sprites["l" .. i]

			Key[i].Alpha = 0
			Key[i].BlendMode = BlendAdd
		end

		Lightning[i] = {}

		Lightning[i].CurrentTime = 0

		Lightning[i].Object = Sprites["beam_k" .. i]
		Lightning[i].Object.BlendMode = BlendAdd
		Lightning[i].Object.Alpha = 1
		Lightning[i].Object.Centered = 1
		Lightning[i].Object.Y = Lightning[i].Object.Y + Lightning[i].Object.Height / 2

		Lightning[i].Update = function(self, delta)
			self.Object.ScaleX = 1 - math.pow(1 - self.CurrentTime / LightingTime, 0.75)
			self.CurrentTime = max(self.CurrentTime - delta, 0)
		end

		Bomb[i] = {}
		Bomb[i].Object = Sprites["bomb_k" .. i]
		Bomb[i].Object.Alpha = 0
		Bomb[i].Object.Centered = 1
		Bomb[i].CurrentTime = 0
		Bomb[i].Update = function(self, delta)
			local lerp = self.CurrentTime / BombTime
			if lerp > 0.5 then
				self.Object.Alpha = lerp
			else
				self.Object.Alpha = 2 * lerp
			end
			self.Object.ScaleX = (1 - lerp) * 2
			self.Object.ScaleY = (1 - lerp) * 2
			self.CurrentTime = max(self.CurrentTime - delta, 0)
		end
	end

	local LifebarValue = Game:GetPlayer(0).LifebarPercent / 100
	HP = Sprites["health"]
	HP.ScaleX = LifebarValue
	HP:SetCropByPixels(0, 352 * LifebarValue, 0, 29)
	Engine:Sort()

	Pulse = Sprites["pulse"]
	Pulse.Y = 485 * YR - Pulse.Height
	Pulse.Lighten = 1
	Pulse.BlendMode = BlendAdd
end

function Cleanup()
end

function OnFullComboEvent()
	IsFullCombo = true
end

function OnFailureEvent()
	if Global:GetCurrentGaugeType(0) ~= LT_GROOVE then
		DoFailAnimation()
		return FailAnimation.Duration
	else
		-- FadeToBlack()
		return 1 --SuccessAnimation.Duration
	end
end

-- When 'enter' is pressed and the game starts, this function is called.
function OnActivateEvent()
end

function HitEvent(JudgmentValue, TimeOff, Lane, IsHold, IsHoldRelease, pn)
    local MapLane = Noteskin[Channels].Map[Lane]

	judgment:OnHit(JudgmentValue, TimeOff, Lane, IsHold, IsHoldRelease, pn)
	combodisplay:OnHit(JudgmentValue, TimeOff, Lane, IsHold, IsHoldRelease, pn)

	Bomb[MapLane].CurrentTime = BombTime
end

function MissEvent(TimeOff, Lane, IsHold, PlayerNumber)
	judgment:OnMiss(TimeOff, Lane, IsHold, PlayerNumber)
	combodisplay:OnMiss(TimeOff, Lane, IsHold, PlayerNumber)
end

function KeyEvent(Key, Code, IsMouseInput)
end

function GearKeyEvent (Lane, IsKeyDown)
    local MapLane = Noteskin[Channels].Map[Lane]
	KeyArray[MapLane] = IsKeyDown

	Lightning[MapLane].Object.Alpha = 1
	Lightning[MapLane].CurrentTime = LightingTime

	if MapLane > 1 then
		if IsKeyDown == 1 then
			Key[MapLane - 1].Alpha = 1
		else
			Key[MapLane - 1].Alpha = 0
		end
	end
end

-- Called when the song is over.
function OnSongFinishedEvent()
	DoSuccessAnimation()
	return SuccessAnimation.Duration
end

function Update(Delta)
	-- Executed every frame.
	local Beat = Game:GetPlayer(0).Beat
	local beatEffect = Beat - math.floor(Beat)

	local SongPercentage = Game:GetPlayer(0).Time / (Game:GetPlayer(0).Duration + 3)

	if Game:GetPlayer(0).Time < 0 then
		SongPercentage = math.pow(Game:GetPlayer(0).Time / -1.5, 2)
	end

	SongPosition.Y = 53 * YR + (402 - SongPosition.Height / 2) * SongPercentage * YR

	local LifebarValue = Game:GetPlayer(0).LifebarPercent / 100
	HP.ScaleX = math.ceil(LifebarValue * 50) / 50
	HP:SetCropByPixels(0, 352 * math.ceil(LifebarValue * 50) / 50, 0, 29)

	Pulse.Alpha = 1 - clamp(beatEffect, 0.5, 1)
	Pulse.LightenFactor = 1 - beatEffect

	if KeyArray[1] == 0 then
		tabletick.Rotation = Beat * 360
	else
		tabletick.Rotation = - Beat * 360
	end

	for i=1,Channels do
		if KeyArray[i] then
			Lightning[i].CurrentTime = LightingTime
		end

		Lightning[i]:Update(Delta)
		Bomb[i]:Update(Delta)
	end

	local CurrentBPM = Game:GetPlayer(0).BPM
	--if CurrentBPM ~= 0 then
		BPMText.Text = string.format("%03d", CurrentBPM)
	--end

	HPText.Text = string.format("%03d%%", LifebarValue * 100)

	scoredisplay:Run(Delta)
	judgment:Run(Delta)
	combodisplay:Run(Delta)
	scoredisplay:Run(Delta)
end
