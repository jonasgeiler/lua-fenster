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
local window_width = 426
local window_height = 240
local window1 = fenster.open(
	window_width,
	window_height,
	'Multi-Window Demo - Press ESC to exit (1)'
)
local window2 = fenster.open(
	window_width,
	window_height,
	'Multi-Window Demo - Press ESC to exit (2)',
	2 -- scale x 2
)

-- Draw a circle on the both windows
draw_circle(window1, window_width / 2, window_height / 2, 30, 0xff0000)
draw_circle(window2, window_width / 2, window_height / 2, 30, 0x0000ff)

-- Draw pixels on both windows
while window1:loop() and window2:loop() and not window1.keys[27] and not window2.keys[27] do
	local x = math.random(0, window_width - 1)
	local y = math.random(0, window_height - 1)
	window1:set(x, y, 0xff0000)
	window2:set(x, y, 0x0000ff)
end
