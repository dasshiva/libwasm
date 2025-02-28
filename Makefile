sources=$(wildcard src/*.c)
objects-c=$(subst src,objs,$(sources))
objects=$(objects-c:.c=.o)
debug_objects=$(subst objs,objs-debug,$(objects))
optimised_objects=$(subst objs,objs-opt, $(objects))
headers=$(wildcard include/*.h)

sample: main.c lib/libwasm.so $(headers)
	$(CC) $< -Llib -lwasm -o $@ -Wl,-rpath=./lib -Iinclude

lib/libwasm.so:  $(objects)
	$(CC) -shared -fPIC -o $@ $^

objs/%.o: src/%.c $(headers)
	$(CC) -c -o $@ $< -Iinclude -fPIC

debug: main.c lib/libdebugwasm.so $(headers)
	$(CC) $< -Llib -ldebugwasm -o $@ -Wl,-rpath=./lib -Iinclude -g

lib/libdebugwasm.so: $(debug_objects)
	$(CC) -shared -fPIC -o $@ $^ -g

objs-debug/%.o: src/%.c $(headers)
	$(CC) -c -o $@ $< -Iinclude -fPIC -g

release: main.c lib/libwasmopt.so $(headers)                                
	$(CC) $< -Llib -lwasmopt -o $@ -Wl,-rpath=./lib -Iinclude -flto=full

lib/libwasmopt.so:  $(optimised_objects)
	$(CC) -shared -fPIC -o $@ $^ -flto=full

objs-opt/%.o: src/%.c $(headers)
	$(CC) -c -o $@ $< -Iinclude -fPIC -O3 -flto=full

.SECONDARY: objects debug_objects
