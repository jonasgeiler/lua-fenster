require('luarocks.loader')
local fenster = require('fenster')

-- Open two windows
local window_width = 320
local window_height = 240
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

-- Draw something on both windows simultaneously
while window1:loop(60) and window2:loop(60) and not window1:key(27) and not window2:key(27) do
	local x = math.random(0, window_width - 1)
	local y = math.random(0, window_height - 1)
	window1:set(x, y, 0xff0000)
	window2:set(x, y, 0x0000ff)
end
