#!/usr/bin/env lua

local fenster = require('fenster')

-- Hack to get the current script directory
local dirname = './' .. (debug.getinfo(1, 'S').source:match('^@?(.*[/\\])') or '') ---@type string
-- Add the project root directory to the package path
package.path = dirname .. '../?.lua;' .. package.path

local ppm = require('demos.lib.ppm')

---Draw a loaded image.
---@param window window*
---@param x integer
---@param y integer
---@param image_pixels integer[][]
---@param image_width integer
---@param image_height integer
local function draw_image(window, x, y, image_pixels, image_width, image_height)
	for iy = 1, image_height do
		local dy = y + (iy - 1)
		for ix = 1, image_width do
			window:set(x + (ix - 1), dy, image_pixels[iy][ix])
		end
	end
end

-- Load either a user-specified image or the default image
local image_path = arg[1] or dirname .. 'assets/uv.ppm'
local image_pixels, image_width, image_height, image_err = ppm.load(image_path)
if not image_pixels or not image_width or not image_height then
	print('Failed to load image: ' .. tostring(image_err))
	return
end

-- Calculate the window scale
local window_scale = 1
while image_width * window_scale < 512 and image_height * window_scale < 512 do
	window_scale = window_scale * 2
end

-- Open a window
local window = fenster.open(
	image_width,
	image_height,
	'Image Demo - Press ESC to exit',
	window_scale
)

-- Draw the image
draw_image(
	window,
	0,
	0,
	image_pixels,
	image_width,
	image_height
)

-- Empty window loop
while window:loop() and not window.keys[27] do
	--
end
