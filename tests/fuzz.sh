#!/bin/sh

# mount rash dir as read only in docker so that you dont make a bunch of random files
# docker run -it -v .:/data:ro silkeh/clang bash

get_rand() {
	dd if=/dev/random of=/tmp/rand bs=4096 count=1 > /dev/null 2> /dev/null
}

get_rand

while ./build/rash < /tmp/rand > /dev/null 2> /tmp/out; do
	get_rand
done
