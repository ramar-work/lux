# libtest - Compile and test the JSON library 
libtest:
	-@rm -rf .libs
	-@mkdir -p .libs/obj
	$(CC) -fPIC -o .libs/obj/ztable.o -c vendor/ztable.c $(IFLAGS)
	$(CC) -fPIC -o .libs/obj/zwalker.o -c vendor/zwalker.c $(IFLAGS)
	$(CC) -fPIC -o .libs/obj/zjson.o -c vendor/zjson.c $(IFLAGS)
	$(CC) -fPIC -o .libs/obj/lua.o -c src/lua.c $(IFLAGS)
	$(CC) -DLUA_LOPEN -fPIC -o .libs/obj/json.o -c src/lua/json.c $(IFLAGS)
	$(CC) -shared -o .libs/json.so .libs/obj/ztable.o .libs/obj/zjson.o .libs/obj/lua.o .libs/obj/json.o .libs/obj/zwalker.o
	cd .libs && lua -l json < ../json.lua
	
