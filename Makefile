sources=$(wildcard src/*.c)
objects-c=$(subst src,objs,$(sources))
objects=$(objects-c:.c=.o)

sample: main.c lib/libwasm.so
	$(CC) $< -Llib -lwasm -o $@ -Wl,-rpath=./lib -Iinclude

lib/libwasm.so:  $(objects)
	$(CC) -shared -fPIC -o $@ $^

objs/%.o: src/%.c
	$(CC) -c -o $@ $< -Iinclude

.SECONDARY: objects
