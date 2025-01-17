PREFIX := /usr/local
INCS := -Isrc/

SANITIZER := 
CFLAGS := -std=gnu17 -D_GNU_SOURCE
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
	$(CC) -o $(BUILD)/$(OUT) $(SANITIZER) $(LDFLAGS) $^  

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(SANITIZER) -c -o $@ $<

clean:
	@rm -r $(BUILD)

install:
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${BUILD}/${OUT} ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/${out}