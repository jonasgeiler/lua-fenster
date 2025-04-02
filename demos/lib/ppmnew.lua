local io = io
local string = string
local math = math
local fenster = require('fenster')

local ppm = {}

---Load a PPM file.
---@param path string
---@return integer[][]? pixels
---@return integer? width
---@return integer? height
---@return string? errmsg
---@nodiscard
function ppm.load(path)
	local ppm_file, ppm_file_err = io.open(path, 'rb')
	if not ppm_file then
		return nil, nil, nil, 'Error opening file: ' .. ppm_file_err
	end

	--[[ Read magic number ]]
	local char1 = ppm_file:read(1) ---@type string?
	if not char1 then
		return nil, nil, nil,
			'Error reading first byte of what is expected to be a Netpbm magic number. Most often, this means your input file is empty'
	end
	local char2 = ppm_file:read(1) ---@type string?
	if not char2 then
		return nil, nil, nil,
			string.format(
				'Error reading second byte of what is expected to be a Netpbm magic number (the first byte was successfully read as 0x%02x). Most often, this means your input file ended early',
				string.byte(char1))
	end
	local format = char1 .. char2
	if format ~= 'P3' and format ~= 'P6' then
		return nil, nil, nil,
			string.format('bad magic number 0x%x - not a PPM file', string.byte(char1) * 256 + string.byte(char2))
	end

	---@return string? char
	---@return string? errmsg
	---@nodiscard
	local function read_char()
		local char = ppm_file:read(1) ---@type string?
		if not char then
			return nil, 'EOF / read error reading a byte'
		end
		if char == '#' then
			repeat
				char = ppm_file:read(1) ---@type string?
				if not char then
					return nil, 'EOF / read error reading a byte'
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
			if not char then
				return nil, char_err
			end
		until char ~= ' ' and char ~= '\t' and char ~= '\n' and char ~= '\r'
		local char_byte = string.byte(char)
		if char_byte < 48 --[['0']] or char_byte > 57 --[['9']] then
			return nil, 'junk in file where an unsigned integer should be'
		end

		local uint = 0
		repeat
			local digit_val = char_byte - 48 --[['0']]
			if uint > 0x7fffffff / 10 then
				return nil, 'ASCII decimal integer in file is too large to be processed.'
			end
			uint = uint * 10
			if uint > 0x7fffffff - digit_val then
				return nil, 'ASCII decimal integer in file is too large to be processed.'
			end
			uint = uint + digit_val ---@type integer

			char, char_err = read_char()
			if not char then
				return nil, char_err
			end
			char_byte = string.byte(char)
		until char_byte < 48 --[['0']] or char_byte > 57 --[['9']]

		return uint, nil
	end

	local cols, cols_err = read_uint()
	if not cols then
		return nil, nil, nil, 'Error reading width: ' .. cols_err
	end
	if cols > 0x7ffffffd then
		return nil, nil, nil, string.format('image width (%u) too large to be processed', cols)
	end
	local rows, rows_err = read_uint()
	if not rows then
		return nil, nil, nil, 'Error reading height: ' .. rows_err
	end
	if rows > 0x7ffffffd then
		return nil, nil, nil, string.format('image height (%u) too large to be processed', rows)
	end

	local maxval, maxval_err = read_uint()
	if not maxval then
		return nil, nil, nil, 'Error reading maxval: ' .. maxval_err
	end
	if maxval > 65535 then
		return nil, nil, nil,
			string.format('maxval of input image (%u) is too large. The maximum allowed by the PPM format is 65535.',
				maxval)
	end
	if maxval == 0 then
		return nil, nil, nil, 'maxval of input image is zero.'
	end

	---@param r any
	---@param g any
	---@param b any
	---@return integer
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
			if not r then return 'Error reading red sample value: ' .. r_err end
			local g, g_err = read_uint()
			if not g then return 'Error reading green sample value: ' .. g_err end
			local b, b_err = read_uint()
			if not b then return 'Error reading blue sample value: ' .. b_err end

			if r > maxval then
				return string.format('Red sample value %u is greater than maxval (%u)', r, maxval)
			end
			if g > maxval then
				return string.format('Green sample value %u is greater than maxval (%u)', g, maxval)
			end
			if b > maxval then
				return string.format('Blue sample value %u is greater than maxval (%u)', b, maxval)
			end

			pixelrow[col] = scaled_rgb(r, g, b)
		end
	end

	---@param pixelrow integer[]
	---@return string? errmsg
	local function read_raw_ppm_row(pixelrow)
		local bytes_per_sample = maxval < 256 and 1 or 2
		local bytes_per_row = cols * 3 * bytes_per_sample
		local row_buffer = ppm_file:read(bytes_per_row) ---@type string?
		if not row_buffer then
			return 'Unexpected EOF reading row of PPM image.'
		end
		if #row_buffer ~= bytes_per_row then
			return string.format('Error reading row. Short read of %u bytes instead of %u', #row_buffer, bytes_per_row)
		end

		-- Read the row buffer
		local buffer_cursor = 1 -- Start at beginning of row_buffer[]
		if bytes_per_sample == 1 then
			for col = 1, cols do
				local r = string.byte(row_buffer, buffer_cursor)
				buffer_cursor = buffer_cursor + 1
				local g = string.byte(row_buffer, buffer_cursor)
				buffer_cursor = buffer_cursor + 1
				local b = string.byte(row_buffer, buffer_cursor)
				buffer_cursor = buffer_cursor + 1

				pixelrow[col] = scaled_rgb(r, g, b)
			end
		else
			-- Two byte samples
			for col = 1, cols do
				local r = string.byte(row_buffer, buffer_cursor) * 256 + string.byte(row_buffer, buffer_cursor + 1)
				buffer_cursor = buffer_cursor + 2
				local g = string.byte(row_buffer, buffer_cursor) * 256 + string.byte(row_buffer, buffer_cursor + 1)
				buffer_cursor = buffer_cursor + 2
				local b = string.byte(row_buffer, buffer_cursor) * 256 + string.byte(row_buffer, buffer_cursor + 1)
				buffer_cursor = buffer_cursor + 2

				pixelrow[col] = scaled_rgb(r, g, b)
			end
		end

		-- Validate raw PPM row
		if maxval == 255 or maxval == 65535 then
			-- There's no way a sample can be invalid, so we don't need to look at the samples individually.
			return nil
		end
		for col = 1, cols do
			local r = fenster.rgb(pixelrow[col])
			local _, g = fenster.rgb(pixelrow[col])
			local _, _, b = fenster.rgb(pixelrow[col])

			if r > maxval then
				return string.format('Red sample value %u is greater than maxval (%u)', r, maxval)
			end
			if g > maxval then
				return string.format('Green sample value %u is greater than maxval (%u)', g, maxval)
			end
			if b > maxval then
				return string.format('Blue sample value %u is greater than maxval (%u)', b, maxval)
			end
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
		elseif format == 'P6' then
			local row_err = read_raw_ppm_row(pixels[row])
			if row_err then
				return nil, nil, nil, 'Error reading pixel data: ' .. row_err
			end
		end
	end

	return pixels, cols, rows, nil
end

return ppm
