.PHONY: all

all: line_reader.a

line_reader.a: main.rs
	rustc --edition 2021 --crate-type=staticlib -g -C opt-level=0 -o $@ $<