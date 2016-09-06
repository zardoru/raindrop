Keys = {}

function Keys:Init()
	self.KeysUp = {}
	self.KeysDown = {}

	self.KeyAtlas = TextureAtlas:skin_new(self.KeysSheet)

	-- Keys
	self.Keys[i] = Engine:CreateObject()
	local obj = self.Keys[i]
	obj.Centered = 1
	obj.X = self.Noteskin["Key" .. i .. "X"]
	obj.Image = self.KeyAtlas.File
	obj.Layer = 27
	self.KeysUp[i] = self.Noteskin["Key" .. i]
	self.KeysDown[i] = self.Noteskin["Key" .. i .. "Down"]

	self.KeyAtlas:SetObjectCrop(obj, self.KeysUp[i])

	obj.Width = self.Noteskin["Key" .. i .. "Width"]
	obj.Height = self.Noteskin.GearHeight

	if self.Player.Upscroll then
		obj.Y = JudgmentLineY - obj.Height / 2 - NoteHeight / 2
		obj.Rotation = 180
	else
		obj.Y = JudgmentLineY + obj.Height / 2 + NoteHeight / 2
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