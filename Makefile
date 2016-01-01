
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

INCLUDES = -I./libaxolotl-cpp/ecc \
           -I./libaxolotl-cpp/exception \
           -I./libaxolotl-cpp/util \
           -I./libaxolotl-cpp/state \
           -I./libaxolotl-cpp/protocol \
           -I./libaxolotl-cpp/groups/ratchet \
           -I./libaxolotl-cpp/groups/state \
           -I./libaxolotl-cpp/kdf \
           -I./libaxolotl-cpp/ratchet \
           -I./libaxolotl-cpp/mem-store \
           -I./libaxolotl-cpp

#           -I./libaxolotl-cpp/sqli-store \

C_SRCS = tinfl.c imgutil.c 
CXX_SRCS = whatsapp-protocol.cc wa_util.cc rc4.cc keygen.cc tree.cc databuffer.cc message.cc wa_purple.cc

C_OBJS = $(C_SRCS:.c=.o)
CXX_OBJS = $(CXX_SRCS:.cc=.o) AxolotlMessages.pb.o

PKG_CONFIG = pkg-config
STRIP = strip
CC = gcc
CXX = g++
LD = $(CXX)
CFLAGS_PURPLE = $(shell $(PKG_CONFIG) --cflags purple)
CFLAGS ?= \
    $(ARCHFLAGS) \
    -O2 \
    -Wall \
    -Wno-unused-function
CFLAGS += \
    -fPIC \
    -DPURPLE_PLUGINS \
    -DPIC \
    $(INCLUDES) \
    $(CFLAGS_PURPLE)

CXXFLAGS += -std=c++11

LIBS_PURPLE = $(shell $(PKG_CONFIG) --libs purple) -lfreeimage ./libaxolotl-cpp/libaxolotl.a -lprotobuf -lcrypto ./libaxolotl-cpp/libcurve25519/libcurve25519.a
LDFLAGS ?= $(ARCHFLAGS)
LDFLAGS += -shared -pipe

AxolotlMessages.pb.h:	AxolotlMessages.proto
	protoc --cpp_out=. AxolotlMessages.proto
AxolotlMessages.pb.cc:	AxolotlMessages.pb.h
	# Do nothing

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
%.o: %.cc AxolotlMessages.pb.h
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) -o $@ $<

$(LIBNAME): $(C_OBJS) $(CXX_OBJS) 
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS_PURPLE)

.PHONY: strip
strip: $(LIBNAME)
	$(STRIP) --strip-unneeded $(LIBNAME)

.PHONY: debug
debug:
	CFLAGS="$$CFLAGS -DDEBUG -g3 -O0" make all

PLUGIN_DIR_PURPLE:=$(shell $(PKG_CONFIG) --variable=plugindir purple)
DATA_ROOT_DIR_PURPLE:=$(shell $(PKG_CONFIG) --variable=datarootdir purple)

.PHONY: install
install: $(LIBNAME)
	install -D $(LIBNAME) $(DESTDIR)$(PLUGIN_DIR_PURPLE)/$(LIBNAME)
	install -D -m 644 whatsapp16.png $(DESTDIR)$(DATA_ROOT_DIR_PURPLE)/pixmaps/pidgin/protocols/16/whatsapp.png
	install -D -m 644 whatsapp22.png $(DESTDIR)$(DATA_ROOT_DIR_PURPLE)/pixmaps/pidgin/protocols/22/whatsapp.png
	install -D -m 644 whatsapp48.png $(DESTDIR)$(DATA_ROOT_DIR_PURPLE)/pixmaps/pidgin/protocols/48/whatsapp.png

.PHONY: uninstall
uninstall: $(LIBNAME)
	rm -f $(DESTDIR)$(PLUGIN_DIR_PURPLE)/$(LIBNAME)
	rm -f $(DESTDIR)$(DATA_ROOT_DIR_PURPLE)/pixmaps/pidgin/protocols/16/whatsapp.png
	rm -f $(DESTDIR)$(DATA_ROOT_DIR_PURPLE)/pixmaps/pidgin/protocols/22/whatsapp.png
	rm -f $(DESTDIR)$(DATA_ROOT_DIR_PURPLE)/pixmaps/pidgin/protocols/48/whatsapp.png

.PHONY: clean
clean:
	-rm -f AxolotlMessages.pb.cc AxolotlMessages.pb.h
	-rm -f *.o
	-rm -f $(LIBNAME)

