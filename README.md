# rash

rash, or really awesome shell (or rat ass shell if you prefer), is a rudimentary shell written in C.

Currently it supports a handful of builtins, but not scripting functionality.

## Building

To build rash, you just need to run `make` in the root of the souce code.

This will make an executable in `./build/rash`, however by default it is a debug build with optimizations turned off.

To make a build without debug symbols that is highly optimized type `make DEBUG=0`

## Contributing

To contibute, make a branch (or fork) with a general name of what you are trying to do. This branch will then be merged (or declined) after review.

Before you commit code it is a good idea to run it through valgrind, and run make using `make ERROR_HELL=1`. This requires clang, and enables all (meaningful) errors. If your code has an error here, you should probably fix it.