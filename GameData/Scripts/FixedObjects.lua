game_require "utils"

FixedObjects = {
}

CreatedObjects = {}
function FixedObjects.CreateFromCSV(file)
	local File = io.open(GetSkinFile(file))
	print ("Opening " .. GetSkinFile(file))
	if File == nil then
		return
	end
	
	for line in File:lines() do
		local tbl = split(line)
		local Object = Engine:CreateObject()
		
		print("Create object " .. tbl[1])
		
		Object.Image = tbl[1]
		Object.X = tbl[2] or 0
		Object.Y = tbl[3] or 0
		Object.Width = tbl[4] or 1
		Object.Height = tbl[5] or 1
		Object.Layer = tbl[6] or Object.Layer
		
		CreatedObjects[#CreatedObjects + 1] = Object
	end
	io.close(File)
end