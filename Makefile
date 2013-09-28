
LIBNAME = libwhatsapp.so
ARCH = ""

ifeq ($(ARCH),i686)
	ARCHFLAGS = -m32
else ifeq ($(ARCH),x86_64)
	ARCHFLAGS = -m64
else
	ARCHFLAGS = 
endif

.PHONY: all
all: $(LIBNAME)

C_SRCS = wa_purple.c
CXX_SRCS = whatsapp-protocol.cc wa_api.cc

C_OBJS = $(C_SRCS:.c=.o)
CXX_OBJS = $(CXX_SRCS:.cc=.o)

STRIP = strip
CC = gcc
CXX = g++
LD = $(CXX)
CFLAGS_PURPLE = $(shell pkg-config --cflags purple)
CFLAGS = \
    $(ARCHFLAGS) \
    -O2 \
    -Wall \
    -fPIC \
    -DPURPLE_PLUGINS \
    -DPIC \
    $(CFLAGS_PURPLE)

LIBS_PURPLE = $(shell pkg-config --libs purple)
LDFLAGS =  $(ARCHFLAGS) -shared -pipe 

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
%.o: %.cc
	$(CXX) -c $(CFLAGS) -o $@ $<

$(LIBNAME): $(C_OBJS) $(CXX_OBJS) 
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS_PURPLE)

.PHONY: strip
strip: $(LIBNAME)
	$(STRIP) --strip-unneeded $(LIBNAME)

.PHONY: debug
debug: CFLAGS += -g
debug: $(LIBNAME)

PLUGIN_DIR_PURPLE:=$(shell pkg-config --variable=plugindir purple)
DATA_ROOT_DIR_PURPLE:=$(shell pkg-config --variable=datarootdir purple)

.PHONY: install
install: $(LIBNAME)
	install -D $(LIBNAME) $(DESTDIR)$(PLUGIN_DIR_PURPLE)/$(LIBNAME)
	install -D whatsapp16.png $(DESTDIR)$(DATA_ROOT_DIR_PURPLE)/pixmaps/pidgin/protocols/16/whatsapp.png
	install -D whatsapp22.png $(DESTDIR)$(DATA_ROOT_DIR_PURPLE)/pixmaps/pidgin/protocols/22/whatsapp.png
	install -D whatsapp48.png $(DESTDIR)$(DATA_ROOT_DIR_PURPLE)/pixmaps/pidgin/protocols/48/whatsapp.png

.PHONY: uninstall
uninstall: $(LIBNAME)
	rm -f $(DESTDIR)$(PLUGIN_DIR_PURPLE)/$(LIBNAME)
	rm -f $(DESTDIR)$(DATA_ROOT_DIR_PURPLE)/pixmaps/pidgin/protocols/16/whatsapp.png
	rm -f $(DESTDIR)$(DATA_ROOT_DIR_PURPLE)/pixmaps/pidgin/protocols/22/whatsapp.png
	rm -f $(DESTDIR)$(DATA_ROOT_DIR_PURPLE)/pixmaps/pidgin/protocols/48/whatsapp.png

.PHONY: clean
clean:
	-rm -f *.o
	-rm -f $(LIBNAME)

