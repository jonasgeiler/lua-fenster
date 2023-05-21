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
   GDI32 = {
      library = "gdi32"
   },
   USER32 = {
       library = "user32"
   },
}

build = {
   type = "builtin",

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
         sources = "src/main.c"
      }
   }
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