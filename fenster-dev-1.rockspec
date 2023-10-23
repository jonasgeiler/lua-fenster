package = "fenster"
version = "dev-1"

source = {
    url = "git+https://github.com/skayo/lua-fenster"
}

description = {
    summary = "WIP",
    detailed = [[
      WIP
   ]],
    homepage = "https://github.com/skayo/lua-fenster",
    license = "MIT"
}

dependencies = {
    "lua >= 5.1"
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
                    incdirs = {
                        "$(GDI32_INCDIR)",
                        "$(USER32_INCDIR)",
                    },
                    libdirs = {
                        "$(GDI32_LIBDIR)",
                        "$(USER32_LIBDIR)",
                    },
                    libraries = {
                        "gdi32",
                        "user32",
                    },
                },
            },
        },
        linux = {
            modules = {
                fenster = {
                    incdirs = {
                        "$(X11_INCDIR)",
                    },
                    libdirs = {
                        "$(X11_LIBDIR)",
                    },
                    libraries = {
                        "X11",
                    },
                },
            },
        },
    },
}


--[[
external_dependencies = {
    GDI32 = {
        library = "gdi32"
    }
}

build = {
    type = "make",

    build_target = "lib",
    build_variables = {
        CFLAGS = "$(CFLAGS) -Wall -Wpedantic -Wextra -std=c99",
        LIBFLAGS = "$(LIBFLAG) -lgdi32",
        LUA_LIBDIR = "$(LUA_LIBDIR)",
        LUA_BINDIR = "$(LUA_BINDIR)",
        LUA_INCDIR = "$(LUA_INCDIR)",
        GDI32_LIBDIR = "$(GDI32_LIBDIR)",
        GDI32_BINDIR = "$(GDI32_BINDIR)",
        GDI32_INCDIR = "$(GDI32_INCDIR)",
        LUA = "$(LUA)",
    },

    install_variables = {
        INST_PREFIX = "$(PREFIX)",
        INST_BINDIR = "$(BINDIR)",
        INST_LIBDIR = "$(LIBDIR)",
        INST_LUADIR = "$(LUADIR)",
        INST_CONFDIR = "$(CONFDIR)",
    },
}
]]
