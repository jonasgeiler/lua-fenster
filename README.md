# lua-fenster

> The most minimal cross-platform GUI library - now in Lua!

[![LuaRocks](https://img.shields.io/luarocks/v/jonasgeiler/fenster?style=for-the-badge&color=%232c3e67)](https://luarocks.org/modules/jonasgeiler/fenster)
[![Downloads](https://img.shields.io/badge/dynamic/xml?url=https%3A%2F%2Fluarocks.org%2Fmodules%2Fjonasgeiler%2Ffenster&query=%2F%2Fdiv%5B%40class%3D%22metadata_columns_inner%22%5D%2Fdiv%5B%40class%3D%22column%22%5D%5Blast()%5D%2Ftext()&style=for-the-badge&label=Downloads&color=%23099dff)](https://luarocks.org/modules/jonasgeiler/fenster)
[![Projects using lua-fenster](https://img.shields.io/badge/Projects_using_lua--fenster-1%2B-2c3e67?style=for-the-badge)](#projects-using-lua-fenster)
[![License](https://img.shields.io/github/license/jonasgeiler/lua-fenster?style=for-the-badge&color=%23099dff)](./LICENSE.md)

A Lua binding for the [fenster](https://github.com/zserge/fenster) GUI library,
providing the most minimal and highly opinionated way to display a
cross-platform 2D canvas. It's basic idea is to give you the simplest means
possible to "just put pixels on the screen" without any of the fancy stuff. As a
nice bonus you also get cross-platform keyboard/mouse input and frame timing in
only a few lines of code.

Read more about the idea behind fenster here:
[Minimal cross-platform graphics - zserge.com](https://zserge.com/posts/fenster/)

> [!NOTE]
> This library is primarily intended for educational purposes and prototyping
> and may not include all the features you would expect from a full-fledged GUI
> library. If you're looking for a more feature-rich library, you might want to
> check out [LÖVE](https://love2d.org/) or [raylib](https://www.raylib.com/).

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

## Usage

Here is a simple example that opens a 500x300 window and draws a red rectangle:

```lua
-- rectangle.lua
local fenster = require("fenster")

local window = fenster.open(500, 300, "Hello fenster!")

while window:loop() and not window.keys[27] do
	window:clear()

	for y = 100, 200 do
		for x = 200, 300 do
			window:set(x, y, 0xff0000)
		end
	end
end
```

To run the example:

```shell
lua rectangle.lua
```

Check out the [demos](./demos) folder for more examples!  
Also, I have compiled a collection of useful snippets in
[this discussion (#11)](https://github.com/jonasgeiler/lua-fenster/discussions/11).
Check them out!

# API

TODO

```
-- Functions --

fenster.open(width: integer, height: integer, title: string?, scale: integer?, targetfps: number?): userdata

fenster.sleep(milliseconds: integer)

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

# Projects using lua-fenster

<!--
If you add your project here, make sure to increase the number in the badge at
the top! It's a little finicky to find the right character in the URL, but
you'll get there eventually.
-->

Here is a list of projects using lua-fenster:

- [jonasgeiler/3d-rasterizer-lua](https://github.com/jonasgeiler/3d-rasterizer-lua)

Feel free to add your project to the list by creating a pull request!