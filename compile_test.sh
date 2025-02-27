#!/usr/bin/bash
if [ "$1" == "all" ]; then
	for file in testing/*
	do
		clang -target wasm32-unknown-none $file -Wl,--allow-undefined -Wl,--entry=main -nostdlib -nostdinc -o .$file.wasm
	done
elif [ "$1" == "test" ]; then
	./sample .testing/*
else
	clang -target wasm32-unknown-none $1 -Wl,--allow-undefined -Wl,--entry=main -nostdlib -nostdinc -o .$1.wasm
fi


