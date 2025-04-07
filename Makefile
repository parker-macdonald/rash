PREFIX := /usr/local

SANITIZER := 
CFLAGS := -std=c99 -D_POSIX_C_SOURCE=200809L
CFLAG_ERRORS := -Werror -Wall -Wvla -Wextra -Wunreachable-code -Wshadow -Wpedantic
LDFLAGS := 
CC := clang
LINTER := clang-tidy

DEBUG := 1

ifeq ($(DEBUG),1)
	CFLAGS += -O0 -g3
else
	CFLAGS += -O3
	LDFLAGS += -s

# use thin lto if using clang
	ifeq ($(CC),clang)
		CFLAGS += -flto=thin
		LDFLAGS += -flto=thin
	else
# use lto if use gcc
		ifeq ($(CC),gcc)
			CFLAGS += -flto
			LDFLAGS += -flto
		endif
	endif
endif

OUT := rash
BUILD := ./build
SRC := ${wildcard src/**/*.c} ${wildcard src/**/**/*.c} ${wildcard src/*.c}
OBJ := ${SRC:%.c=$(BUILD)/%.o}


.PHONY: all clean build install lint
.DEFAULT: all

all: build

lint:
	$(LINTER) $(SRC) -checks=-*,bugprone-*cert-*,clang-analyzer-*,performance-*,portability-*,misc-*,readability-*,-readability-function-cognitive-complexity -warnings-as-errors=* -- $(INCS) $(CFLAGS)

build: $(BUILD)/$(OUT)

$(BUILD)/$(OUT): $(OBJ)
	$(CC) -o $(BUILD)/$(OUT) $(SANITIZER) $(LDFLAGS) $^  

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CFLAG_ERRORS) $(SANITIZER) -c -o $@ $<

clean:
	@rm -r $(BUILD)

install:
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${BUILD}/${OUT} ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/${out}