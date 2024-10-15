-- Heart Demo by turgenevivan
-- https://github.com/turgenevivan

local fenster = require('fenster')

local width = 144
local height = 144
local window_scale = 4
local fps = 30

local window = fenster.open(width, height, 'Heart Demo - Press ESC to exit, 1-4 to change type', window_scale, fps)

-- Heart shape function
local function heart(x, y)
	-- https://mathworld.wolfram.com/HeartCurve.html
	-- Convert window coordinates to mathematical coordinates, using window center as origin
	local xp = (x - width / 2) / (width / 3) -- Scale factor for x
	local yp = (height / 2 - y) / (height / 3) -- Scale and invert y because y increases downwards

	-- Heart equation: (x^2 + y^2 - 1)^3 = x^2 * y^3
	return (xp ^ 2 + yp ^ 2 - 1) ^ 3 - xp ^ 2 * yp ^ 3 <= 0
end

-- Define the heart shape function with a pattern using level sets
local function heart_with_pattern(x, y)
	-- https://en.wikipedia.org/wiki/Level_set
	-- Convert window coordinates to mathematical coordinates, using window center as origin
	local xp = (x - width / 2) / (width / 3) -- Scale factor for x
	local yp = (height / 2 - y) / (height / 3) -- Scale and invert y

	-- Heart level set function
	local z = xp * xp + yp * yp - 1
	local f = z * z * z - xp * xp * yp * yp * yp

	-- Mapping result to color intensity (like characters in the original code)
	if f <= 0.0 then
		-- Map 'f' value to grayscale intensity based on the same idea as character mapping
		local intensity = math.floor((f * -8.0) % 8) -- Same concept as indexing into characters
		local shades = {
			0x330000,          -- Dark red (represents '@')
			0x660000,          -- Darker red (represents '%')
			0x990000,          -- Medium dark red (represents '#')
			0xcc0000,          -- Medium red (represents '*')
			0xff0000,          -- Bright red (represents '+')
			0xff3333,          -- Light red (represents '=')
			0xff6666,          -- Lighter red (represents ':')
			0xffffff,          -- White (represents '.')
		}
		return shades[intensity + 1] -- Return color based on intensity
	else
		return 0xffffff        -- Background white
	end
end

-- Define the heart shape function f(x, y, z)
local function f(x, y, z)
	local a = x * x + (9.0 / 4.0) * y * y + z * z - 1
	return a * a * a - x * x * z * z * z - (9.0 / 80.0) * y * y * z * z * z
end

-- Define h(x, z) function to find the surface at a given x and z
local function h(x, z)
	for y = 1.0, 0.0, -0.001 do
		if f(x, y, z) <= 0.0 then
			return y
		end
	end
	return 0.0
end


local function draw_heart_with_pattern()
	for x = 0, width - 1 do
		for y = 0, height - 1 do
			local color = heart_with_pattern(x, y)
			window:set(x, y, color) -- Set pixel color based on the level set function using hex
		end
	end
end

local function draw_heart()
	for x = 0, width - 1 do
		for y = 0, height - 1 do
			if heart(x, y) then
				window:set(x, y, 0xff0000) -- Set red color for the heart shape
			end
		end
	end
end

local function draw_heart3d()
	for sy = 0, height - 1 do       -- same
		local z = 1.5 - sy * 3.0 / height -- Map screen coordinates to z
		for sx = 0, width - 1 do
			local x = sx * 3.0 / width - 1.5 -- Map screen coordinates to x
			local v = f(x, 0.0, z)  -- Evaluate the heart equation
			local r = 0             -- Red channel value

			if v <= 0.0 then
				local y0 = h(x, z)                          -- Get the y value at (x, z)
				local ny = 0.0011                           -- Small offset for normal calculation
				local nx = h(x + ny, z) - y0                -- Calculate the normal x component
				local nz = h(x, z + ny) - y0                -- Calculate the normal z component
				local nd = 1.0 / math.sqrt(nx * nx + ny * ny + nz * nz) -- Normalize
				local d = (nx + ny - nz) * nd * 0.5 + 0.5   -- Calculate brightness
				r = math.floor(d * 255.0)                   -- Map to 0-255 range
			end

			-- Ensure red channel is clamped between 0 and 255
			r = math.max(0, math.min(255, r))

			-- Set the pixel color (R, G, B)
			window:set(sx, sy, (r * 2 ^ 16) + (0 * 2 ^ 8) + 0) -- Only red color
		end
	end
end

-- Draw the heart shape with color
local function draw_heart3d_animate(t)
	-- Heart scaling based on time (to create the pulsing effect)
	local s = math.sin(t)
	local a = s * s * s * s * 0.2 -- Scaling factor based on sine wave

	for sy = 0, height - 1 do
		local z = 1.5 - sy * 3.0 / height -- Map screen coordinates to z
		z = z * (1.2 - a)           -- animate
		for sx = 0, width - 1 do
			local x = sx * 3.0 / width - 1.5 -- Map screen coordinates to x
			x = x * (1.2 + a)       -- animate
			local v = f(x, 0.0, z)  -- Evaluate the heart equation
			local r = 0             -- Red channel value

			if v <= 0.0 then
				local y0 = h(x, z)                          -- Get the y value at (x, z)
				local ny = 0.0011                           -- Small offset for normal calculation
				local nx = h(x + ny, z) - y0                -- Calculate the normal x component
				local nz = h(x, z + ny) - y0                -- Calculate the normal z component
				local nd = 1.0 / math.sqrt(nx * nx + ny * ny + nz * nz) -- Normalize
				local d = (nx + ny - nz) * nd * 0.5 + 0.5   -- Calculate brightness
				r = math.floor(d * 255.0)                   -- Map to 0-255 range
			end

			-- Ensure red channel is clamped between 0 and 255
			r = math.max(0, math.min(255, r))

			-- Set the pixel color (R, G, B)
			local color = fenster.rgb(r, 0, 0)
			window:set(sx, sy, color) -- Only red color
		end
	end
end


local draw_menu = {
	[1] = draw_heart,
	[2] = draw_heart_with_pattern,
	[3] = draw_heart3d,
	[4] = draw_heart3d_animate,
}
local draw_type = 1

local t = 0
while window:loop() and not window.keys[27] do
	window:clear()

	if window.keys[string.byte('1')] then
		draw_type = 1
	elseif window.keys[string.byte('2')] then
		draw_type = 2
	elseif window.keys[string.byte('3')] then
		draw_type = 3
	elseif window.keys[string.byte('4')] then
		t = 0
		draw_type = 4
	end

	draw_menu[draw_type](t)
	t = t + 3 * window.delta -- Update time
end
