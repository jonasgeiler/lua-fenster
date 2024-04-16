# lua-fenster

WIP

## Installation

From LuaRocks server:
```shell
luarocks install fenster
```

From source:
```shell
git clone https://github.com/jonasgeiler/lua-fenster.git
cd lua-fenster
luarocks make
```

## Notes (WIP)

API:
```
-- Functions --

fenster.open(width: integer, height: integer, title: string?, scale: integer?, targetfps: number?): userdata

fenster.sleep(millis: integer)

fenster.time(): integer

fenster.rgb(redorcolor: integer, green?: integer, blue?: integer): integer,integer?,integer?


-- Methods --

fenster.close(window: userdata)
window:close()

fenster.loop(window: userdata): boolean
window:loop(): boolean

fenster.set(window: userdata, x: integer, y: integer, color: integer)
window:set(x: integer, y: integer, color: integer)

fenster.get(window: userdata, x: integer, y: integer): integer
window:get(x: integer, y: integer): integer

fenster.clear(window: userdata, color: integer?)
window:clear(color: integer?)


-- Properties --

window.keys: boolean[]

window.delta: number

window.mousex: integer
window.mousey: integer
window.mousedown: boolean

window.modcontrol: boolean
window.modshift: boolean
window.modalt: boolean
window.modgui: boolean

window.width: integer

window.height: integer

window.title: string

window.scale: integer

window.targetfps: number
```

## Useful Snippets

### Basic window loop

A simple window loop that runs at maximum 60 FPS and closes when the Escape key
is pressed:

```lua
while window:loop() and not window.keys[27] do
	-- Draw stuff here
end 
```

### Fill buffer

Fills the entire window buffer with a specific color:

```lua
-- Normally you would store window:width() and window:height() in variables
for y = 0, window.height - 1 do
	for x = 0, window.width - 1 do
		window:set(x, y, color)
	end
end
```

> [!IMPORTANT]
> Pixel coordinates are zero-based. Unlike Lua tables they start at `0`.

### Key code map

Here is a very basic key to key code map:

```lua
local keys = {
	backspace = 8,
	tab = 9,
	enter = 10,
	escape = 27,
	space = 32,
	["'"] = 39,
	[','] = 44,
	['-'] = 45,
	['.'] = 46,
	['/'] = 47,
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
	['['] = 91,
	['\\'] = 92,
	[']'] = 93,
	['`'] = 96,
}

-- Usage
if window.keys[keys.enter] then
	-- Enter key is pressed
end 
```

Also check out the [text demo](./demos/text.lua) for a more advanced keyboard
usage.

### Calculating FPS

Calculating the FPS to measure performance is super easy with the window delta:
```lua
local fps = 1 / window.delta
```

### Drawing rectangles

Check out the [moving demo](./demos/moving.lua)!
It includes a function to draw rectangles.

### Drawing circles

Check out the [multi-window demo](./demos/multi-window.lua)!
It includes a function to draw circles.

### Drawing lines

Check out the [paint demo](./demos/paint.lua)!
It includes a function to draw lines.

### Drawing text

Check out the [text demo](./demos/text.lua)!
It includes the microknight font and a function to draw text.

### Loading and drawing images

Images are a bit more complex to load and draw. I recommend using a very simple
image format like [Netpbm PPM (P3 or P6)](https://en.wikipedia.org/wiki/Netpbm)
to store the image in a file.
To load such a PPM (P6) image, check out the [image demo](./demos/image.lua).  
Keep in mind that the image I have included in the
demo, [found here](./demos/assets/uv.ppm), has been cleaned of any comments in
the header since those are a bit complicated to handle (and the code I wrote for
the image demo won't handle them).

Here's the command for ImageMagick to convert a PNG to PPM:
```shell
magick image.png -depth 8 image.ppm
```
You could use GIMP as well, but unfortunately GIMP adds a comment to the header 
of the PPM file, which makes it a bit more complicated to use compared to 
ImageMagick.
