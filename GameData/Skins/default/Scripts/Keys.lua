Keys = {}

function Keys:Init()
	self.KeysUp = {}
	self.KeysDown = {}

	self.KeyAtlas = TextureAtlas:skin_new(self.KeysSheet)

	-- Keys
	self.Keys[i] = Engine:CreateObject()
	local obj = self.Keys[i]
	obj.Centered = 1
	obj.X = Noteskin[Channels]["Key" .. i .. "X"]
	obj.Image = self.KeyAtlas.File
	obj.Layer = 27
	self.KeysUp[i] = Noteskin[Channels]["Key" .. i]
	self.KeysDown[i] = Noteskin[Channels]["Key" .. i .. "Down"]

	self.KeyAtlas:SetObjectCrop(obj, self.KeysUp[i])

	obj.Width = Noteskin[Channels]["Key" .. i .. "Width"]
	obj.Height = Noteskin[Channels].GearHeight

	if Upscroll == 1 then
		obj.Y = JudgmentLineY - obj.Height / 2 - NoteHeight / 2
		obj.Rotation = 180
	else
		obj.Y = JudgmentLineY + obj.Height / 2 + NoteHeight / 2
	end
end

function Keys:GearKeyEvent(i, IsKeyDown)
	i = i + 1
	if IsKeyDown == 1 then
		self.KeyAtlas:SetObjectCrop(self.Keys[i], self.KeysDown[i])
	else
		self.KeyAtlas:SetObjectCrop(self.Keys[i], self.KeysUp[i])
	end
end
