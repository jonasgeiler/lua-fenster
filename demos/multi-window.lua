local fenster = require('fenster')

---Draw a filled circle
---@param window userdata
---@param x number
---@param y number
---@param radius number
---@param color number
local function draw_circle(window, x, y, radius, color)
	for dy = -radius, radius do
		for dx = -radius, radius do
			if dx * dx + dy * dy < radius * radius then
				window:set(x + dx, y + dy, color)
			end
		end
	end
end

-- Open two windows
local window_width = 640
local window_height = 480
local window1 = fenster.open(
	'Multi-Window Demo - Press ESC to exit (1)',
	window_width,
	window_height
)
local window2 = fenster.open(
	'Multi-Window Demo - Press ESC to exit (2)',
	window_width,
	window_height
)

-- Draw a circle on the both windows
draw_circle(window1, window_width / 2, window_height / 2, 50, 0xff0000)
draw_circle(window2, window_width / 2, window_height / 2, 50, 0x0000ff)

-- Draw pixels on both windows
while window1:loop(60) and window2:loop(60) and not window1:key(27) and not window2:key(27) do
	local x = math.random(0, window_width - 1)
	local y = math.random(0, window_height - 1)
	window1:set(x, y, 0xff0000)
	window2:set(x, y, 0x0000ff)
end
