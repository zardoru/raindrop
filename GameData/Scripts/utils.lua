function split(line)
	local restable = {}
	local i = 1
	for k in string.gmatch(line, "([^,]+)") do
		restable[i] = k
		i = i + 1
	end
	
	return restable
end