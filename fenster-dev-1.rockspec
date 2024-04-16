rockspec_format = '3.0'
package = 'fenster'
version = 'dev-1' -- this will be replaced by the release workflow
source = {
	url = 'git+https://github.com/jonasgeiler/lua-fenster',
	branch = 'main', -- this will be replaced by the release workflow
}
description = {
	summary = 'The most minimal cross-platform GUI library - now in Lua! (WIP)',
	detailed = '' ..
		'A Lua binding for the fenster (https://github.com/zserge/fenster) ' ..
		'C library, providing a minimal cross-platform GUI library for ' ..
		'creating windows and drawing pixels. This binding is currently in ' ..
		'development and not feature-complete.',
	license = 'MIT',
	homepage = 'https://github.com/jonasgeiler/lua-fenster',
	issues_url = 'https://github.com/jonasgeiler/lua-fenster/issues',
	maintainer = 'Jonas Geiler',
	labels = {
		'gui', 'graphics', 'pixel', 'windowing', '2d', 'drawing', 'window',
		'framebuffer', 'gui-library',
	},
}
dependencies = {
	'lua >= 5.1, <= 5.4',
}
build_dependencies = {
	platforms = {
		macosx = {
			'luarocks-build-extended',
		},
	},
}
external_dependencies = {
	platforms = {
		linux = {
			X11 = {
				library = 'X11',
			},
		},
		win32 = {
			GDI32 = {
				library = 'gdi32',
			},
			USER32 = {
				library = 'user32',
			},
		},
	},
}
build = {
	type = 'builtin',
	modules = {
		fenster = {
			sources = 'src/main.c',
		},
	},
	platforms = {
		linux = {
			modules = {
				fenster = {
					libraries = {
						'X11',
					},
					incdirs = {
						'$(X11_INCDIR)',
					},
					libdirs = {
						'$(X11_LIBDIR)',
					},
				},
			},
		},
		win32 = {
			modules = {
				fenster = {
					libraries = {
						'gdi32',
						'user32',
					},
					incdirs = {
						'$(GDI32_INCDIR)',
						'$(USER32_INCDIR)',
					},
					libdirs = {
						'$(GDI32_LIBDIR)',
						'$(USER32_LIBDIR)',
					},
				},
			},
		},
		macosx = {
			type = 'extended',
			modules = {
				fenster = {
					variables = {
						LIBFLAG_EXTRAS = {
							'-framework', 'Cocoa',
						},
					},
				},
			},
		},
	},
}
test = {
	type = 'busted',
	flags = '--verbose',
}