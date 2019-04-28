libdir.x86_64 := $(shell if [ -d "/usr/lib/x86_64-linux-gnu" ]; then echo "/usr/lib/x86_64-linux-gnu"; else echo "/usr/lib64"; fi )
libdir.i686   := $(shell if [ -d "/usr/lib/i386-linux-gnu" ]; then echo "/usr/lib/i386-linux-gnu"; else echo "/usr/lib"; fi )
libdir.macos  := /usr/local/lib

ISNOTMACOS := $(shell uname -a | grep "Darwin" >/dev/null ; echo $$? )

ifeq ($(ISNOTMACOS), 0)
	MACHINE := macos
	CFLAGS := -bundle
else
	MACHINE := $(shell uname -m)
	CFLAGS := -shared
endif

libdir = $(libdir.$(MACHINE))/geany

all: prepare build

prepare:
	cd ./lloyd-yajl-66cb08c && bash configure
	cp ./lloyd-yajl-66cb08c/src/api/yajl_common.h ./lloyd-yajl-66cb08c/build/yajl-2.1.0/include/yajl
	cd ./lloyd-yajl-66cb08c && make distro

build:
	gcc -DLOCALEDIR=\"\" -DGETTEXT_PACKAGE=\"zhgzhg\" -c ./geany_json_prettifier.c -fPIC `pkg-config --cflags geany`
	gcc geany_json_prettifier.o -o jsonprettifier.so "./lloyd-yajl-66cb08c/build/yajl-2.1.0/lib/libyajl_s.a" $(CFLAGS) `pkg-config --libs geany`

install: globaluninstall globalinstall localuninstall

uninstall: globaluninstall

globaluninstall:
	rm -f "$(libdir)/jsonprettifier.so"
	rm -f $(libdir)/jsonprettifier.*
	rm -f $(libdir)/json_prettifier.*

globalinstall:
	cp -f ./jsonprettifier.so "$(libdir)/jsonprettifier.so"
	chmod 755 "$(libdir)/jsonprettifier.so"

localinstall: localuninstall
	cp -f ./jsonprettifier.so "$(HOME)/.config/geany/plugins/jsonprettifier.so"
	chmod 755 "$(HOME)/.config/geany/plugins/jsonprettifier.so"

localuninstall:
	rm -f "$(HOME)/.config/geany/plugins/jsonprettifier.so"

clean:
	rm -f ./jsonprettifier.so
	rm -f ./geany_json_prettifier.o
	rm -f ./lloyd-yajl-66cb08c/Makefile
	rm -fr ./lloyd-yajl-66cb08c/build
