game_require "librd"

Histogram = {}
Histogram.__index = Histogram

function Histogram:GenerateObjects(w, h)
  local skeep = self.Player.Scorekeeper
  local pnt_cnt = skeep.HistogramPointCount

	with  (self, {
  	item_size = w / pnt_cnt,
  	Objects = {},
  	Width = w,
  	Height = h,
	  X = 0,
  	Y = 0
	})

  for i=1,pnt_cnt do
    local ref = ScreenObject {
    	Texture = "Global/white.png",
    	Width = self.item_size
		}

    self.Objects[i] = ref
    -- x, y, and h are set in updatepoints
  end

  self.centerSep = Engine:CreateObject()
  self:UpdatePoints()
end

function Histogram:UpdatePoints()
  local skeep = self.Player.Scorekeeper
  local top_point = skeep.HistogramHighestPoint

  for k,ref in pairs(self.Objects) do
    -- change this 128 by the amount of ms + 1 the histogram covers
    -- basically floor(point_count / 2)

    ref.X = self.item_size * (k - 1) + self.X
    ref.Height = self.Height * skeep:GetHistogramPoint(k - 128) / top_point
    ref.Y = self.Y - ref.Height + self.Height

    if (k - 128) == 0 then
			with (self.centerSep, {
	      Texture = "Global/white.png",
	      Width = self.item_size,
	      Height = self.Height,
	      X = ref.X,
	      Y = self.Y
			})
    end

  end
end

function Histogram:SetLayer(layer)
  if not layer then return end

  for k,v in pairs(self.Objects) do
    v.Layer = layer
  end
  self.centerSep.Layer = layer - 1
  if self.bg then
    self.bg.layer = layer - 2
  end
end

function Histogram:SetColor(r, g, b)
  for k,v in pairs(self.Objects) do
    v.Red = r or v.Red
    v.Green = g or v.Green
    v.Blue = b or v.Blue
  end

  self.centerSep.Red = r * 0.5 or self.centerSep.Red
  self.centerSep.Green = g * 0.5 or self.centerSep.Green
  self.centerSep.Blue = b * 0.5 or self.centerSep.Blue
end

function Histogram:SetPosition(x, y)
  self.X = x or self.X
  self.Y = y or self.Y
  self:UpdatePoints()
end

function Histogram:SetBackground(image)
  if self.bg then
    self.bg.Image = image
  else
    self.bg = Engine:CreateObject()
    self.bg.X = self.X
    self.bg.Y = self.Y
    self.bg.Texture = image
    self.bg.Width = self.Width
    self.bg.Height = self.Height
  end

  return self.bg
end

function Histogram:new(player, width, height, layer)
  local out = {}
  local skeep = player.Scorekeeper

  setmetatable(out, self)
  out.Player = player
  out:GenerateObjects(width or skeep.HistogramPointCount * 1.15, height or 100)
  out:SetLayer(layer or 16)
  return out
end
