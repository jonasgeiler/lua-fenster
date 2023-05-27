require('luarocks.loader')
local fenster = require('fenster')

local WIDTH = 1920
local HEIGHT = 1080

local f = fenster.open('Test', WIDTH, HEIGHT)

for y = 0, HEIGHT - 1 do
    for x = 0, WIDTH - 1 do
        f:set(x, y, math.random(0, 0xFFFFFF))
    end
end

while f:loop(60) do
    for y = 0, HEIGHT - 1 do
        for x = 0, WIDTH - 1 do
            f:set(x, y, math.random(0, 0xFFFFFF))
        end
    end

    if f:key(27) then
        break
    end
end

f:close()
