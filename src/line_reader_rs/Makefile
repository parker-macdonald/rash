.PHONY: all

all: line_reader.a

rust_eh_personality.o: rust_eh_personality.c
	gcc -c -o $@ $<

line_reader.a: main.rs rust_eh_personality.o
	rustc --edition 2021 --crate-type=staticlib -g -C opt-level=0 -C panic="abort" -o $@ $<
	ar q $@ rust_eh_personality.o