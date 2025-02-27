sources=$(wildcard src/*.c)
objects-c=$(subst src,objs,$(sources))
objects=$(objects-c:.c=.o)
debug_objects=$(subst objs,objs-debug,$(objects))

sample: main.c lib/libwasm.so
	$(CC) $< -Llib -lwasm -o $@ -Wl,-rpath=./lib -Iinclude

lib/libwasm.so:  $(objects)
	$(CC) -shared -fPIC -o $@ $^

objs/%.o: src/%.c
	$(CC) -c -o $@ $< -Iinclude -fPIC

debug: main.c lib/libdebugwasm.so
	$(CC) $< -Llib -ldebugwasm -o $@ -Wl,-rpath=./lib -Iinclude -g

lib/libdebugwasm.so: $(debug_objects)
	$(CC) -shared -fPIC -o $@ $^ -g

objs-debug/%.o: src/%.c
	$(CC) -c -o $@ $< -Iinclude -fPIC -g

.SECONDARY: objects debug_objects
