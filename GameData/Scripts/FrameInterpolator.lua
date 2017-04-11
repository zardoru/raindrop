game_require "TextureAtlas"

FrameInterpolator = {
	DefaultFramerate = 60,
	TotalFrames = 0
}

FrameInterpolator.__index = FrameInterpolator

function FrameInterpolator.FilenameAssign(fn)
	return (fn + 1) .. ".png"
end

function FrameInterpolator:New(sprite_file, duration, object)
	local out = {}
	setmetatable(out, self)

	out.SpriteSheet = TextureAtlas:new(GetSkinFile(sprite_file))

	if out.SpriteSheet == nil then
		print "Sprite sheet couldn't be found. Sorry."
		return nil
	end

	out.Object = object or Engine:CreateObject()
	local i = 0
	for k, v in pairs(out.SpriteSheet.Sprites) do
		i = i + 1
	end

	out.TotalFrames = i or 1
	out.Duration = duration or 1

	out.CurrentTime = 0

	out.Object.Texture = out.SpriteSheet.File or "null"

	out:Update(0)

	return out
end

function FrameInterpolator:GetFrameAtFrac(frac)
	local Frame = math.floor(frac * (self.TotalFrames - 1))
	return Frame
end

function FrameInterpolator:SetFraction(frac)
	local fn = self.FilenameAssign(self:GetFrameAtFrac(frac))
	self.SpriteSheet:SetObjectCrop(self.Object, fn)
end

function FrameInterpolator:GetLerp()
	return clamp(self.CurrentTime / self.Duration, 0, 1)
end

function FrameInterpolator:Update(delta)
	self.CurrentTime = self.CurrentTime + delta
	self:SetFraction(self:GetLerp())
end
