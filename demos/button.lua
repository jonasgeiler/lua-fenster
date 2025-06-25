-- button.lua
local fenster = require("fenster");

counter = 0;

-- Button factory (no label)
function button(window, x, y, width, height, color1, color2, on_click_function)
	return function()
		local isMouseHovering = (window.mousex > x
			and window.mousex < x + width
			and window.mousey > y
			and window.mousey < y + height);

		if isMouseHovering then
			for yPos = y, (y + height) do
				for xPos = x, (x + width) do
					window:set(xPos, yPos, color2);
				end
			end
			fenster.sleep(10);
		else
			for yPos = y, (y + height) do
				for xPos = x, (x + width) do
					window:set(xPos, yPos, color1);
				end
			end
			fenster.sleep(10);
		end

		if (on_click_function ~= nil
		and window.mousedown
		and isMouseHovering) then
			on_click_function();
		else
			return fenster.sleep(10);
		end

		-- Do nothing until you let go of the mouse button
		if window.mousedown then
			while window:loop() and window.mousedown do
				fenster.sleep(10);
			end
		end
	end
end

function on_click()
	counter = counter + 1;
	print(counter);
end

function on_click2()
	print("Hi fenster!");
end

function main()
	local window = fenster.open(320, 240, "Button Test");
	local btn1 = button(window, 110, 96, 100, 12, 0xcccccc, 0x8888ff, on_click);
	local btn2 = button(window, 110, 116, 100, 12, 0xcccccc, 0x8888ff, on_click2);
	window:clear(0x161616);
	while window:loop() and not window.keys[27] do
		btn1();
		btn2();
	end
end

main();
