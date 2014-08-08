TextureAtlas = {}
TextureAtlas.__index = TextureAtlas

function SetCropToAtlas(Atlas, Sprite)
	local Tab = Atlas.Sprites[Sprite]

	Obj.CropByPixels(Tab.x, Tab.y, Tab.x + Tab.w, Tab.y + Tab.h)
end

function AssignFrames(Atlas, Filename)
	local File = io.open(Filename)

	if File ~= nil then

		for line in File:lines() do
			if Atlas.File == nil then
				Atlas.File = line;
				Atlas.Sprites = {}
			else
				local restable = {}
				local i = 1
				for k in string.gmatch(line, "([^,]+)") do
					restable[i] = k
					i = i + 1
				end

				Sprite = {x = restable[2], y = restable[3], w = restable[4], h = restable[5]}

				Atlas.Sprites[restable[1]] = Sprite
			end
		end

		io.close(File)
	else
		print("Error opening " .. Filename .. ". Atlas won't be constructed.")
	end
end

function TextureAtlas:new(filename)
	local NewAtlas = {}
	setmetatable(NewAtlas, TextureAtlas)
	AssignFrames(NewAtlas, filename)
	return NewAtlas
end
