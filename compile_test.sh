#!/usr/bin/bash
if [ "$1" == "all" ]; then
	for file in testing/*
	do
		clang -target wasm32-unknown-none $file -Wl,--allow-undefined -nostdlib -nostdinc -o .$file.wasm -mcpu=mvp
	done
elif [ "$1" == "test" ]; then
	./sample .testing/*
else
	clang -target wasm32-unknown-none $1 -Wl,--allow-undefined -nostdlib -nostdinc -o .$1.wasm -mcpu=mvp
fi


