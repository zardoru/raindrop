game_require "utils"

FixedObjects = { XRatio = 1, YRatio = 1 }

Sprites = {}

function FixedObjects.CreateObjectFromParameters(tbl, constants)
	local Object = Engine:CreateObject()
		
	print("Create object " .. tbl[2])
		
	local name = tbl[2]
	Object.Image = constants[tbl[1]] or tbl[1] or 0
	Object.X = (constants[tbl[3]] or tbl[3] or 0) * FixedObjects.XRatio
	Object.Y = (constants[tbl[4]] or tbl[4] or 0) * FixedObjects.YRatio
	Object.Width = (constants[tbl[5]] or tbl[5] or 1) * FixedObjects.XRatio
	Object.Height = (constants[tbl[6]] or tbl[6] or 1) * FixedObjects.YRatio
	Object.Layer = constants[tbl[7]] or tbl[7] or Object.Layer
	
	-- Object.Centered = constants[tbl[8]] or tbl[8] or 0
	Object.Rotation = constants[tbl[9]] or tbl[9] or 0
	Object.ScaleX = constants[tbl[10]] or tbl[10] or 1
	Object.ScaleY = constants[tbl[11]] or tbl[11] or 1
	
	Sprites[name] = Object
end

function FixedObjects.CreateFromCSV(file, constants)
	local File = io.open(GetSkinFile(file))
	print ("Opening " .. GetSkinFile(file))
	
	constants = constants or {}
	
	if File == nil then
		return
	end
	
	for line in File:lines() do
		if line[1] ~= "#" then -- Not a comment
			local tbl = split(line)
			if tbl then
				FixedObjects.CreateObjectFromParameters(tbl, constants)
			end
		end
	end
	
	io.close(File)
end