#
# !IMPORTANT!
# I created this file for CLion and it works on my machine, but you should rather use `luarocks build` to build the library.
# I really don't have any experience with Makefiles, so I can't guarantee that it will work on your machine.
#

LD ?= gcc
LDFLAGS ?= -shared
X11_LIBDIR ?= /usr/lib64

CC ?= gcc
CFLAGS ?= -O2 -fPIC
LUA_INCDIR ?= /usr/include
X11_INCDIR ?= /usr/include

all: fenster.so

fenster.so: src/main.o
	$(LD) $(LDFLAGS) $(LIBFLAG) -o $@ $< -L$(X11_LIBDIR) -lX11

src/main.o: src/main.c
	$(CC) $(CFLAGS) -I$(LUA_INCDIR) -c $< -o $@ -I$(X11_INCDIR)

clean:
	rm -f src/main.o fenster.so