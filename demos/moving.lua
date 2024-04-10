local fenster = require('fenster')

---Draw a filled rectangle
---@param window userdata
---@param x number
---@param y number
---@param width number
---@param height number
---@param color number
local function draw_rectangle(window, x, y, width, height, color)
	local start_x = math.floor(x)
	local start_y = math.floor(y)
	local end_x = math.floor(x + width - 1)
	local end_y = math.floor(y + height - 1)
	for i = start_x, end_x do
		for j = start_y, end_y do
			window:set(i, j, color)
		end
	end
end

-- Rectangle settings
local rect_width = 30
local rect_height = 20
local rect_speed = 50
local rect_colors = {
	0xff0000,
	0x00ff00,
	0xffff00,
	0x0000ff,
	0xff00ff,
	0x00ffff,
}

-- Open a window
local window_width = 256
local window_height = 144
local window_scale = 2
local window = fenster.open(
	window_width,
	window_height,
	'Moving Demo - Press ESC to exit',
	window_scale
)

-- Draw a moving rectangle
local rect_x, rect_y = 0, 0
local rect_dir_x, rect_dir_y = 1, 1
local rect_color_index = 1
while window:loop() and not window.keys[27] do
	local delta = window.delta

	-- Clear the screen for redraw
	window:clear()

	-- Move the rectangle
	rect_x = rect_x + rect_speed * rect_dir_x * delta
	rect_y = rect_y + rect_speed * rect_dir_y * delta

	-- Check if the rectangle would be out of bounds
	if rect_x < 0 or rect_x + rect_width >= window_width then
		-- Flip the x direction
		rect_dir_x = -rect_dir_x

		-- Change the color
		rect_color_index = (rect_color_index % #rect_colors) + 1

		-- Recalculate the x position with the new direction
		rect_x = rect_x + rect_speed * rect_dir_x * delta
	end
	if rect_y < 0 or rect_y + rect_height >= window_height then
		-- Flip the y direction
		rect_dir_y = -rect_dir_y

		-- Change the color
		rect_color_index = (rect_color_index % #rect_colors) + 1

		-- Recalculate the y position with the new direction
		rect_y = rect_y + rect_speed * rect_dir_y * delta
	end

	-- Draw the rectangle
	draw_rectangle(window, rect_x, rect_y, rect_width, rect_height, rect_colors[rect_color_index])
end
