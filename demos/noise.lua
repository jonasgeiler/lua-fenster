local fenster = require('fenster')

-- Open a window
local window_width = 256
local window_height = 144
local window = fenster.open(
	'Noise Demo - Press ESC to exit',
	window_width,
	window_height,
	4
)

-- Generate noise
while window:loop(60) and not window:key(27) do
	for y = 0, window_height - 1 do
		for x = 0, window_width - 1 do
			window:set(x, y, math.random(0x000000, 0xffffff))
		end
	end
end
