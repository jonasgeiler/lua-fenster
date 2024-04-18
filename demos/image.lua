local fenster = require('fenster')

-- Load image
local image_path = './' .. (
-- Get the relative directory of the script
	debug.getinfo(1, 'S').source:match('^@?(.*[/\\])') or ''
) .. 'assets/uv.ppm'
local image = assert(io.open(image_path, 'rb'))

-- Minimal PPM (P6) file format parser (won't handle comments!)
local image_type = image:read(2)
assert(image_type == 'P6', 'Invalid image type: ' .. tostring(image_type))
assert(image:read(1), 'Invalid image header') -- Whitespace
local image_width = image:read('*number')
assert(image_width, 'Invalid image width: ' .. tostring(image_width))
assert(image:read(1), 'Invalid image header') -- Whitespace
local image_height = image:read('*number')
assert(image_height, 'Invalid image height: ' .. tostring(image_height))
assert(image:read(1), 'Invalid image header') -- Whitespace
local image_max_color = image:read('*number')
assert(
	image_max_color == 255,
	'Invalid image maximum color: ' .. tostring(image_max_color)
)
assert(image:read(1), 'Invalid image header') -- Whitespace
local image_buffer = {}
while true do
	local r_raw = image:read(1)
	local g_raw = image:read(1)
	local b_raw = image:read(1)
	if not r_raw or not g_raw or not b_raw then
		break
	end

	local r = string.byte(r_raw)
	local g = string.byte(g_raw)
	local b = string.byte(b_raw)
	table.insert(image_buffer, fenster.rgb(r, g, b))
end

-- Open a window
local window = fenster.open(
	image_width,
	image_height,
	'Image Demo - Press ESC to exit'
)

-- Draw the image
for y = 0, image_height - 1 do
	for x = 0, image_width - 1 do
		local index = (y * image_width) + x + 1
		window:set(x, y, image_buffer[index])
	end
end

-- Empty window loop
while window:loop() and not window.keys[27] do
	--
end
