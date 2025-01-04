PREFIX := /usr/local
INCS := -Isrc/

CFLAGS := -std=c17 -pedantic -Wall
CFLAG_ERRORS := -Werror -Wall -Wextra -Wunreachable-code -Wshadow -Wpedantic
LDFLAGS := $(INCS)
CC := clang

DEBUG := 1

ifeq ($(DEBUG),1)
	CFLAGS += -O0 -g3
else
	CFLAGS += -O3
	LDFLAGS += -s
endif

OUT := rash
BUILD := ./build
SRC := ${wildcard src/**/*.c} ${wildcard src/*.c}
OBJ := ${SRC:%.c=$(BUILD)/%.o}


.PHONY: all clean build
.DEFAULT: all

all: build

build: $(BUILD)/$(OUT)

$(BUILD)/$(OUT): $(OBJ)
	$(CC) -o $(BUILD)/$(OUT) $(LDFLAGS) $^  

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@rm -r $(BUILD)