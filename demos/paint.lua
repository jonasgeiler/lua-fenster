local fenster = require('fenster')

-- Open a window
local window_width = 320
local window_height = 240
local window = fenster.open(
	'Paint Demo - Press ESC to exit',
	window_width,
	window_height
)

-- Pretty simple main loop, just draw pixels where the mouse is clicked
while window:loop(60) and not window:key(27) do
    local mouse_x, mouse_y, mouse_down = window:mouse()
	if mouse_down then
        window:set(mouse_x, mouse_y, 0xffffff)
    end
end

window:close()
