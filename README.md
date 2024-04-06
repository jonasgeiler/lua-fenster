# lua-fenster

WIP

## Useful Snippets

### Basic window loop
A simple window loop that runs at maximum 60 FPS and closes when the Escape key
is pressed:
```lua
while window:loop(60) and not window:key(27) do
    -- Draw stuff here
end 
```

### Fill buffer
Fills the entire window buffer with a specific color:
```lua
for y = 0, window_height - 1 do
    for x = 0, window_width - 1 do
        window:set(x, y, color)
    end
end
```
> [!IMPORTANT]
> Pixel coordinates are zero-based. Unlike Lua tables they start at `0`.

### RGB colors
There are many ways to calculate the 24-bit color value from RGB components.
Here are a few examples:
```lua
-- Simple maths (recommended in Lua 5.1/5.2 and LuaJIT)
window:set(x, y, r * 0x10000 + g * 0x100 + b)

-- Native bitwise operators (recommended in Lua 5.3+)
window:set(x, y, (r << 16) | (g << 8) | b)

-- Using the bit32 library (included with Lua 5.2/5.3, also backported to Lua 5.1)
window:set(x, y, bit32.bor(bit32.lshift(r, 16), bit32.lshift(g, 8), b))

-- Using the bit library (included with LuaJIT, also compatible with Lua 5.1/5.2)
window:set(x, y, bit.bor(bit.lshift(r, 16), bit.lshift(g, 8), b))
```

### Key code map
Here is a very basic key to key code map:
```lua
local keys = {
	backspace = 8,
	tab = 9,
	enter = 10,
	escape = 27,
	space = 32,
	['0'] = 48,
	['1'] = 49,
	['2'] = 50,
	['3'] = 51,
	['4'] = 52,
	['5'] = 53,
	['6'] = 54,
	['7'] = 55,
	['8'] = 56,
	['9'] = 57,
	a = 65,
	b = 66,
	c = 67,
	d = 68,
	e = 69,
	f = 70,
	g = 71,
	h = 72,
	i = 73,
	j = 74,
	k = 75,
	l = 76,
	m = 77,
	n = 78,
	o = 79,
	p = 80,
	q = 81,
	r = 82,
	s = 83,
	t = 84,
	u = 85,
	v = 86,
	w = 87,
	x = 88,
	y = 89,
	z = 90,
}

-- Usage
if window:key(keys.enter) then
    -- Enter key is pressed
end 
```

### Drawing text
Check out the [text demo](./demos/text.lua)!
It includes the microknight font and a function to draw text.
