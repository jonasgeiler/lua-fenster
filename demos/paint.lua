local fenster = require('fenster')

---Draw a line between two points (window boundary version)
---@param window table
---@param x0 number
---@param y0 number
---@param x1 number
---@param y1 number
---@param color number
local function draw_line(window, x0, y0, x1, y1, color)
	local window_width = window:width()
	local window_height = window:height()
	local dx = math.abs(x1 - x0)
	local dy = math.abs(y1 - y0)
	local sx = x0 < x1 and 1 or -1
	local sy = y0 < y1 and 1 or -1
	local err = (dx > dy and dx or -dy) / 2
	local e2
	while true do
		if x0 >= 0 and x0 < window_width and y0 >= 0 and y0 < window_height then
			window:set(x0, y0, color)
		end

		if x0 == x1 and y0 == y1 then
			break
		end

		e2 = err
		if e2 > -dx then
			err = err - dy
			x0 = x0 + sx
		end
		if e2 < dy then
			err = err + dx
			y0 = y0 + sy
		end
	end
end

-- Open a window
local window_width = 640
local window_height = 480
local window = fenster.open(
	'Paint Demo - Press ESC to exit',
	window_width,
	window_height
)

-- Main loop
local last_mouse_x, last_mouse_y
while window:loop(60) and not window:key(27) do
    local mouse_x, mouse_y, mouse_down = window:mouse()

	-- Check if the mouse is pressed
	if mouse_down then
		-- Check if the last mouse position is not set
		if not last_mouse_x and not last_mouse_y then
			-- Use the current mouse position as the "last mouse position" (line starting point)
			last_mouse_x, last_mouse_y = mouse_x, mouse_y
		end

		-- Draw a line between the last mouse position and the current mouse position
		draw_line(window, last_mouse_x, last_mouse_y, mouse_x, mouse_y, 0xffffff)

		-- Update the last mouse position
		last_mouse_x, last_mouse_y = mouse_x, mouse_y
	elseif last_mouse_x and last_mouse_y then
		-- Reset the last mouse position when the mouse is released
		last_mouse_x, last_mouse_y = nil, nil
	end
end
