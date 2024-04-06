rockspec_format = "3.0"
package = "fenster"
version = "dev-1"
source = {
	url = "git+https://github.com/jonasgeiler/lua-fenster",
	branch = "main",
}
description = {
	summary = "WIP",
	detailed = [[
      WIP
   ]],
	license = "MIT",
	homepage = "https://github.com/jonasgeiler/lua-fenster",
	issues_url = "https://github.com/jonasgeiler/lua-fenster/issues",
	maintainer = "Jonas Geiler",
	labels = {
		'gui',
		'graphics',
	}
}
supported_platforms = {
	"!maxosx", -- macOS is not supported yet
}
dependencies = {
	"lua >= 5.1, <= 5.4",
}
external_dependencies = {
	platforms = {
		win32 = {
			GDI32 = {
				library = "gdi32",
			},
			USER32 = {
				library = "user32",
			},
		},
		linux = {
			X11 = {
				library = "X11",
			},
		},
	},
}
build = {
	type = "builtin",
	modules = {
		fenster = {
			sources = "src/main.c",
		},
	},
	platforms = {
		win32 = {
			modules = {
				fenster = {
					libraries = {
						"gdi32",
						"user32",
					},
					incdirs = {
						"$(GDI32_INCDIR)",
						"$(USER32_INCDIR)",
					},
					libdirs = {
						"$(GDI32_LIBDIR)",
						"$(USER32_LIBDIR)",
					},
				},
			},
		},
		linux = {
			modules = {
				fenster = {
					libraries = {
						"X11",
					},
					incdirs = {
						"$(X11_INCDIR)",
					},
					libdirs = {
						"$(X11_LIBDIR)",
					},
				},
			},
		},
	},
}
