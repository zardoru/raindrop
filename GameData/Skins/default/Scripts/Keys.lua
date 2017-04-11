Keys = {
    KeysSheet = "VSRG/keys.csv"
  }

function Keys:Init()
	self.KeysUp = {}
	self.KeysDown = {}

	self.KeyAtlas = TextureAtlas:skin_new(self.KeysSheet)

  self.Keys = {}
  
	-- Keys
  for i=1, self.Player.Channels do
    self.Keys[i] = Engine:CreateObject()
    local obj = self.Keys[i]
    obj.Centered = 1
    obj.X = self.Noteskin["Key" .. i .. "X"]
    obj.Texture = self.KeyAtlas.File
    obj.Layer = 27
    self.KeysUp[i] = self.Noteskin["Key" .. i]
    self.KeysDown[i] = self.Noteskin["Key" .. i .. "Down"]

    self.KeyAtlas:SetObjectCrop(obj, self.KeysUp[i])

    obj.Width = self.Noteskin["Key" .. i .. "Width"]
    obj.Height = self.Noteskin.GearHeight

    if self.Player.Upscroll then
      obj.Y = self.Player.JudgmentY - obj.Height / 2 - self.Noteskin.NoteHeight / 2
      obj.Rotation = 180
    else
      obj.Y = self.Player.JudgmentY + obj.Height / 2 + self.Noteskin.NoteHeight / 2
    end
  end
end

librd.make_new(Keys, Keys.Init)

function Keys:GearKeyEvent(i, IsKeyDown, pn)
  if pn ~= self.Player.Number then
    return
  end
  
	if IsKeyDown then
		self.KeyAtlas:SetObjectCrop(self.Keys[i], self.KeysDown[i])
	else
		self.KeyAtlas:SetObjectCrop(self.Keys[i], self.KeysUp[i])
	end
end

return Keys