game_require "librd"

FixedObjects = { XRatio = 1, YRatio = 1 }

function FixedObjects:CreateObjectFromParameters(tbl, constants)
	local Object = Engine:CreateObject()

	print("Create object " .. tbl[2])

	local name = tbl[2]
	-- table.dump(constants)
	Object.Texture = constants[tbl[1]] or tbl[1] or 0
	with (Object, {
		X = (constants[tbl[3]] or tbl[3] or 0) * self.XRatio,
		Y = (constants[tbl[4]] or tbl[4] or 0) * self.YRatio,
		Width = (constants[tbl[5]] or tbl[5] or 1) * self.XRatio,
		Height = (constants[tbl[6]] or tbl[6] or 1) * self.YRatio,
		Layer = constants[tbl[7]] or tbl[7] or Object.Layer,
		Rotation = constants[tbl[9]] or tbl[9] or 0,
		ScaleX = constants[tbl[10]] or tbl[10] or 1,
		ScaleY = constants[tbl[11]] or tbl[11] or 1
	})

	-- Object.Centered = constants[tbl[8]] or tbl[8] or 0
	self.Sprites[name] = Object
end

function FixedObjects:CreateFromCSV(file, constants)
	local File = io.open(GetSkinFile(file))
	print ("Opening " .. GetSkinFile(file))

	constants = constants or {}

	if File == nil then
		return
	end

	for line in File:lines() do
		if line[1] ~= "#" then -- Not a comment
			local tbl = string.split(line)
			if tbl then
				-- table.dump(tbl)
				self:CreateObjectFromParameters(tbl, constants)
			end
		end
	end

	io.close(File)
end

function FixedObjects:new()
	local ret = {}
	ret.Sprites = {}
	setmetatable(ret, self)
	return ret
end

FixedObjects.__index = FixedObjects

return FixedObjects
