all:
#	gcc -o ./yajl ./yajl.c "./lloyd-yajl-66cb08c/build/yajl-2.1.0/lib/libyajl_s.a"
	
	gcc -DLOCALEDIR=\"\" -DGETTEXT_PACKAGE=\"zhgzhg\" -c ./geany_json_prettifier.c -fPIC `pkg-config --cflags geany`
	gcc geany_json_prettifier.o -o json_prettifier.so "./lloyd-yajl-66cb08c/build/yajl-2.1.0/lib/libyajl_s.a" -shared `pkg-config --libs geany`
#	cp -f ./json_prettifier.so /usr/lib/geany

clean:
	rm ./json_prettifier.so
