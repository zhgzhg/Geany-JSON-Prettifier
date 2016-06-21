libdir.x86_64 := $(shell if [ -d "/usr/lib/x86_64-linux-gnu" ]; then echo "/usr/lib/x86_64-linux-gnu"; else echo "/usr/lib64"; fi )
libdir.i686   := $(shell if [ -d "/usr/lib/i386-linux-gnu" ]; then echo "/usr/lib/i386-linux-gnu"; else echo "/usr/lib"; fi )

MACHINE := $(shell uname -m)

libdir = $(libdir.$(MACHINE))

all: prepare build

prepare:
	cd ./lloyd-yajl-66cb08c && bash configure
	cp ./lloyd-yajl-66cb08c/src/api/yajl_common.h ./lloyd-yajl-66cb08c/build/yajl-2.1.0/include/yajl
	cd ./lloyd-yajl-66cb08c && make distro
	
build:	
	gcc -DLOCALEDIR=\"\" -DGETTEXT_PACKAGE=\"zhgzhg\" -c ./geany_json_prettifier.c -fPIC `pkg-config --cflags geany`
	gcc geany_json_prettifier.o -o jsonprettifier.so "./lloyd-yajl-66cb08c/build/yajl-2.1.0/lib/libyajl_s.a" -shared `pkg-config --libs geany`

install: uninstall startinstall

startinstall:
	cp -f ./jsonprettifier.so $(libdir)/geany
	chmod 755 $(libdir)/geany/jsonprettifier.so

uninstall:
	rm -f $(libdir)/geany/jsonprettifier.*
	rm -f $(libdir)/geany/json_prettifier.*	

clean:
	rm -f ./jsonprettifier.so
	rm -f ./geany_json_prettifier.o
	rm -f ./lloyd-yajl-66cb08c/Makefile
	rm -fr ./lloyd-yajl-66cb08c/build
