local fenster = require('fenster')

---Map a value from one range to another
---@param value number
---@param start1 number
---@param stop1 number
---@param start2 number
---@param stop2 number
---@return number
---@nodiscard
local function map(value, start1, stop1, start2, stop2)
	return start2 + (stop2 - start2) * ((value - start1) / (stop1 - start1))
end

-- Open a window
local window_width = 144
local window_height = 144
local window = fenster.open(
	'Fractal Demo - Press ESC to exit',
	window_width,
	window_height,
	4
)

-- Fractal settings
local fractal_depth = 64
local generation_infinity = 16

-- Get range of the fractal
local range = 2
local x_min = 0 - range
local x_max = 0 + range
local y_min = 0 - range
local y_max = 0 + range

-- Display the fractal
local last_frame_time = fenster.time()
local angle = 0
while window:loop(60) and not window:key(27) do
	-- Calculate the delta time, which is needed for FPS independent rotation
	local curr_frame_time = fenster.time()
	local delta = (curr_frame_time - last_frame_time) / 1000
	last_frame_time = curr_frame_time

	-- Draw the fractal
	for y = 0, window_height - 1 do
		for x = 0, window_width - 1 do
			local real = map(x, 0, window_width, x_min, x_max)
			local imag = map(y, 0, window_height, y_min, y_max)

			local depth = 0
			while depth < fractal_depth do
				local re = real * real - imag * imag
				local im = 2 * real * imag

				real = re + math.cos(angle)
				imag = im + math.sin(angle)

				if math.abs(real + imag) > generation_infinity then
					break
				end
				depth = depth + 1
			end

			local color = 0x000000
			if depth < fractal_depth then
				color = depth * 32 % 256
			end
			window:set(x, y, color)
		end
	end

	-- Rotate the fractal
	angle = angle + 1 * delta
end
