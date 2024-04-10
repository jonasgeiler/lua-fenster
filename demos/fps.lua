local fenster = require('fenster')

-- Open a window
local window_width = 320
local window_height = 240
local window = fenster.open(
	window_width,
	window_height,
	'FPS Demo - Press ESC to exit'
)

local last_frame_time = fenster.time()
local fps = 0
while window:loop() and not window.keys[27] do
	local delta_time = (fenster.time() - last_frame_time) / 1000
	last_frame_time = fenster.time()

	fps = 1 / delta_time
	print('FPS: ' .. fps)
end
