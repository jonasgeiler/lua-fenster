local io = io
local string = string
local math = math
local fenster = require('fenster')

local ppm = {}
ppm.INT_MAX = 0x7fffffff
ppm.CHAR_0 = 48 -- '0' in ASCII
ppm.CHAR_9 = 57 -- '9' in ASCII

---Load a PPM file. (Adapted from Netbpm library v10.86.45)
---@param path string
---@return integer[][]? pixels
---@return integer? width
---@return integer? height
---@return string? errmsg
---@nodiscard
function ppm.load(path)
	local ppm_file, ppm_file_err = io.open(path, 'rb')
	if not ppm_file then
		return nil, nil, nil, 'Failed to open file: ' .. ppm_file_err
	end

	-- Read magic number
	local char1 = ppm_file:read(1) ---@type string?
	if not char1 then
		return nil, nil, nil, 'File is empty or invalid: missing magic number'
	end
	local char2 = ppm_file:read(1) ---@type string?
	if not char2 then
		return nil, nil, nil, string.format(
			'Incomplete magic number: first byte read as 0x%02x, but second byte is missing',
			string.byte(char1)
		)
	end
	local format = char1 .. char2
	if format ~= 'P3' and format ~= 'P6' then
		if char1 == 'P' then
			-- Pressumably another Netpbm format...
			return nil, nil, nil, string.format(
				'Invalid magic number 0x%x (%s): not a PPM file (P3/P6)',
				string.byte(char1) * 256 + string.byte(char2),
				format
			)
		else
			return nil, nil, nil,
				string.format('Invalid magic number 0x%x: not a PPM file', string.byte(char1) * 256 + string.byte(char2))
		end
	end

	---@return string? char
	---@return string? errmsg
	---@nodiscard
	local function read_char()
		local char = ppm_file:read(1) ---@type string?
		if not char then
			return nil, 'Unexpected end of file while reading a byte'
		end
		if char == '#' then
			repeat
				char = ppm_file:read(1) ---@type string?
				if not char then
					return nil, 'Unexpected end of file while reading a comment'
				end
			until char == '\n' or char == '\r'
		end
		return char, nil
	end

	---@return integer? uint
	---@return string? errmsg
	---@nodiscard
	local function read_uint()
		local char, char_err ---@type string?, string?
		repeat
			char, char_err = read_char()
			if not char then return nil, char_err end
		until char ~= ' ' and char ~= '\t' and char ~= '\n' and char ~= '\r'
		local char_byte = string.byte(char)
		if char_byte < ppm.CHAR_0 or char_byte > ppm.CHAR_9 then
			return nil, 'Expected an unsigned integer but found invalid data'
		end

		local uint = 0
		repeat
			local digit_val = char_byte - ppm.CHAR_0
			if uint > ppm.INT_MAX / 10 then
				return nil, 'Integer value is too large to process'
			end
			uint = uint * 10
			if uint > ppm.INT_MAX - digit_val then
				return nil, 'Integer value is too large to process'
			end
			uint = uint + digit_val ---@type integer

			char, char_err = read_char()
			if not char then return nil, char_err end
			char_byte = string.byte(char)
		until char_byte < ppm.CHAR_0 or char_byte > ppm.CHAR_9

		return uint, nil
	end

	-- Read width
	local cols, cols_err = read_uint()
	if not cols then
		return nil, nil, nil, 'Failed to read image width: ' .. cols_err
	end
	if cols > ppm.INT_MAX - 2 then
		return nil, nil, nil, string.format('Image width (%u) exceeds the maximum supported size', cols)
	end

	-- Read height
	local rows, rows_err = read_uint()
	if not rows then
		return nil, nil, nil, 'Failed to read image height: ' .. rows_err
	end
	if rows > ppm.INT_MAX - 2 then
		return nil, nil, nil, string.format('Image height (%u) exceeds the maximum supported size', rows)
	end

	-- Read max value
	local maxval, maxval_err = read_uint()
	if not maxval then
		return nil, nil, nil, 'Failed to read max color value: ' .. maxval_err
	end
	if maxval > 65535 then
		return nil, nil, nil, string.format('Max color value (%u) exceeds the PPM format limit of 65535', maxval)
	end
	if maxval == 0 then
		return nil, nil, nil, 'Max color value cannot be zero'
	end

	---Scale RGB values from 0 to maxval, to 0 to 255, and then turn them into a single integer.
	---@param r integer
	---@param g integer
	---@param b integer
	---@return integer
	---@nodiscard
	local function scaled_rgb(r, g, b)
		if maxval == 255 then
			return fenster.rgb(r, g, b)
		end

		return fenster.rgb(
			math.floor(r * 255 / maxval),
			math.floor(g * 255 / maxval),
			math.floor(b * 255 / maxval)
		)
	end

	---@param pixelrow integer[]
	---@return string? errmsg
	---@nodiscard
	local function read_ppm_row(pixelrow)
		for col = 1, cols do
			local r, r_err = read_uint()
			if not r then
				return 'Failed to read red value: ' .. r_err
			end
			if r > maxval then
				return string.format('Red value (%u) exceeds max color value (%u)', r, maxval)
			end

			local g, g_err = read_uint()
			if not g then
				return 'Failed to read green value: ' .. g_err
			end
			if g > maxval then
				return string.format('Green value (%u) exceeds max color value (%u)', g, maxval)
			end

			local b, b_err = read_uint()
			if not b then
				return 'Failed to read blue value: ' .. b_err
			end
			if b > maxval then
				return string.format('Blue value (%u) exceeds max color value (%u)', b, maxval)
			end

			pixelrow[col] = scaled_rgb(r, g, b)
		end

		return nil
	end

	---@param pixelrow integer[]
	---@return string? errmsg
	---@nodiscard
	local function read_raw_ppm_row(pixelrow)
		local bytes_per_sample = maxval < 256 and 1 or 2
		local bytes_per_row = cols * 3 * bytes_per_sample
		local row_buffer = ppm_file:read(bytes_per_row) ---@type string?
		if not row_buffer then
			return 'Unexpected end of file while reading a row of pixel data'
		end
		if #row_buffer ~= bytes_per_row then
			return string.format('Incomplete row: expected %u bytes but got %u', bytes_per_row, #row_buffer)
		end

		-- Read the row buffer
		local buffer_cursor = 1 -- Starts at beginning of row_buffer
		for col = 1, cols do
			local r, g, b ---@type integer, integer, integer
			if bytes_per_sample == 1 then
				r = string.byte(row_buffer, buffer_cursor)
				buffer_cursor = buffer_cursor + 1 ---@type integer
				g = string.byte(row_buffer, buffer_cursor)
				buffer_cursor = buffer_cursor + 1
				b = string.byte(row_buffer, buffer_cursor)
				buffer_cursor = buffer_cursor + 1
			else
				-- Two byte samples
				r = string.byte(row_buffer, buffer_cursor) * 256 + string.byte(row_buffer, buffer_cursor + 1)
				buffer_cursor = buffer_cursor + 2 ---@type integer
				g = string.byte(row_buffer, buffer_cursor) * 256 + string.byte(row_buffer, buffer_cursor + 1)
				buffer_cursor = buffer_cursor + 2
				b = string.byte(row_buffer, buffer_cursor) * 256 + string.byte(row_buffer, buffer_cursor + 1)
				buffer_cursor = buffer_cursor + 2
			end

			-- Validate colors
			if r > maxval then
				return string.format('Red value (%u) exceeds max color value (%u)', r, maxval)
			end
			if g > maxval then
				return string.format('Green value (%u) exceeds max color value (%u)', g, maxval)
			end
			if b > maxval then
				return string.format('Blue value (%u) exceeds max color value (%u)', b, maxval)
			end

			pixelrow[col] = scaled_rgb(r, g, b)
		end

		return nil
	end

	local pixels = {} ---@type integer[][]
	for row = 1, rows do
		pixels[row] = {} ---@type integer[]
		if format == 'P3' then
			local row_err = read_ppm_row(pixels[row])
			if row_err then
				return nil, nil, nil, 'Error reading pixel data: ' .. row_err
			end
		else -- format == 'P6'
			local row_err = read_raw_ppm_row(pixels[row])
			if row_err then
				return nil, nil, nil, 'Error reading raw pixel data: ' .. row_err
			end
		end
	end

	return pixels, cols, rows, nil
end

return ppm
