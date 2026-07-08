Init = function()
	Notes.BarlineWidth = -1
end

Update = function()
	Notes.BarlineStartX = -1
end

DrawHoldBody = function()
	Notes.NoteScreenSize = -1
end

return {
	Init = function()
		Notes.BarlineWidth = 123
	end,
	Update = function(delta, beat)
		Notes.BarlineStartX = delta * 10 + beat
	end,
	DrawHoldBody = function(lane, loc, size, active_level)
		Notes.NoteScreenSize = lane + loc + size + active_level
	end
}
