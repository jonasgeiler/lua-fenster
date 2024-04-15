local fenster = require('fenster')

-- Open a window
local window_width = 320
local window_height = 240
local window = fenster.open(
	window_width,
	window_height,
	'FPS Demo - Press ESC to exit'
)

local fps = 0
while window:loop() and not window.keys[27] do
	fps = 1.0 / window.delta
	print('FPS: ' .. fps)
end
