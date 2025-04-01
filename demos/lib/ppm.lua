local tostring = tostring
local io = io
local string = string
local math = math
local fenster = require('fenster')

local ppm = {}

---Load a PPM file.
---@param path string
---@return integer[][]|nil, integer|nil, integer|nil, string|nil
---@nodiscard
function ppm.load(path)
	local ppm_file, ppm_file_err = io.open(path, 'rb')
	if not ppm_file then
		return nil, nil, nil, 'Failed to open PPM file: ' .. tostring(ppm_file_err)
	end

	---Read the next line and trim whitespace.
	---@return string|nil, string|nil
	---@nodiscard
	local function read_line()
		local line = ppm_file:read('*line') ---@type string|nil
		if not line then
			return nil, 'Failed to read line, possibly early EOF'
		end
		line = string.match(line, '^%s*(.-)%s*$') ---@type string|nil
		if not line then
			return nil, 'Failed to trim line'
		end
		return line, nil
	end

	---Parse a string as an integer, while making sure it is positive, not NaN/Inf and not a float.
	---@param raw string|nil
	---@param name string
	---@return integer|nil, string|nil
	---@nodiscard
	local function parse_int(raw, name)
		local number = tonumber(raw)
		if not number or number <= 0 or number ~= number
			or number == math.huge or number == -math.huge or number ~= math.floor(number) then
			local number_str = tostring(number)
			if #number_str > 20 then
				number_str = string.sub(number_str, 1, 20) .. '...'
			end
			return nil, 'Invalid ' .. tostring(name) .. ': ' .. number_str
		end
		return number --[[ @as integer ]], nil
	end

	--[[ Read header ]]
	local line, line_err = read_line()
	if not line then return nil, nil, nil, line_err end
	-- Read the magic number
	local magic_number ---@type string|nil
	magic_number, line = string.match(line, '^(%S+)%s*(.*)') ---@type string|nil
	if magic_number ~= 'P6' then
		if #magic_number > 20 then
			magic_number = string.sub(magic_number, 1, 20) .. '...'
		end
		return nil, nil, nil, 'Unsupported magic number, expected P6 (PPM Binary/Raw): ' .. magic_number
	end
	-- Skip comments, if line has ended
	if not line or line == '' then
		repeat
			line, line_err = read_line()
			if not line then return nil, nil, nil, line_err end
		until string.sub(line, 1, 1) ~= '#'
	end
	-- Read the width
	local raw_width ---@type string|nil
	raw_width, line = string.match(line, '^(%d+)%s*(.*)') ---@type string|nil, string|nil
	local width, width_err = parse_int(raw_width, 'width')
	if not width then return nil, nil, nil, width_err end
	-- Skip comments, if line has ended
	if not line or line == '' then
		repeat
			line, line_err = read_line()
			if not line then return nil, nil, nil, line_err end
		until string.sub(line, 1, 1) ~= '#'
	end
	-- Read the height
	local raw_height ---@type string|nil
	raw_height, line = string.match(line, '^(%d+)%s*(.*)') ---@type string|nil, string|nil
	local height, height_err = parse_int(raw_height, 'height')
	if not height then return nil, nil, nil, height_err end
	-- Skip comments, if line has ended
	if not line or line == '' then
		repeat
			line, line_err = read_line()
			if not line then return nil, nil, nil, line_err end
		until string.sub(line, 1, 1) ~= '#'
	end
	-- Read the maximum color
	local raw_maximum_color = string.match(line, '^(%d+)') ---@type string|nil
	local maximum_color, maximum_color_err = parse_int(raw_maximum_color, 'maximum color')
	if not maximum_color then return nil, nil, nil, maximum_color_err end
	-- TODO: Support up to 65535
	if maximum_color > 255 then
		return nil, nil, nil, 'Unsupported maximum color, expected below or equal 255: ' .. tostring(maximum_color)
	end

	--[[ Read pixel data ]]
	local pixels = {} ---@type integer[][]
	for y = 1, height do
		pixels[y] = {} ---@type integer[]
		for x = 1, width do
			local r_raw = ppm_file:read(1) ---@type string|nil
			local g_raw = ppm_file:read(1) ---@type string|nil
			local b_raw = ppm_file:read(1) ---@type string|nil
			if not r_raw or not g_raw or not b_raw then
				return nil, nil, nil, 'Failed to read pixel data, possibly early EOF'
			end

			local r = string.byte(r_raw)
			if r > maximum_color then
				return nil, nil, nil, 'Invalid pixel data, red value out of range: ' .. tostring(r)
			end
			local g = string.byte(g_raw)
			if g > maximum_color then
				return nil, nil, nil, 'Invalid pixel data, green value out of range: ' .. tostring(g)
			end
			local b = string.byte(b_raw)
			if b > maximum_color then
				return nil, nil, nil, 'Invalid pixel data, blue value out of range: ' .. tostring(b)
			end
			if maximum_color ~= 255 then
				r = math.floor(r * 255 / maximum_color)
				g = math.floor(g * 255 / maximum_color)
				b = math.floor(b * 255 / maximum_color)
			end
			pixels[y][x] = fenster.rgb(r, g, b)
		end
	end

	ppm_file:close()
	return pixels, width, height, nil
end

return ppm
