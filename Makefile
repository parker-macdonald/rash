VERSION=0.7.9

PREFIX := /usr/local

SANITIZER :=
INCLUDE := -Isrc/
CFLAGS := -std=c17 -D_DEFAULT_SOURCE -DVERSION=\"$(VERSION)\" $(INCLUDE)
CFLAG_ERRORS := -Werror -Wall -Wvla -Wextra -Wunreachable-code -Wshadow -Wpedantic -Wconversion
LDFLAGS := -lm -lunwind
CC := clang
LINTER := clang-tidy

ERROR_HELL := 0
DEBUG := 1
STATIC := 0

ifneq ($(SANITIZER),)
	CFLAGS += -fsanitize=$(SANITIZER)
	LDFLAGS += -fsanitize=$(SANITIZER)
endif

ifeq ($(STATIC),1)
	LDFLAGS += -static
endif

ifeq ($(ERROR_HELL),1)
	CC = clang
	CFLAG_ERRORS = -Werror -Weverything -Wno-unsafe-buffer-usage -Wno-declaration-after-statement -Wno-switch-default -Wno-switch-enum -Wno-disabled-macro-expansion -Wno-padded -Wno-pre-c11-compat -Wno-implicit-void-ptr-cast -Wno-date-time -Wno-c++-keyword -Wno-unreachable-code-return
endif

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
HEADERS := ${wildcard src/**/*.h} ${wildcard src/**/**/*.h} ${wildcard src/*.h}
OBJ := ${SRC:%.c=$(BUILD)/%.o}


.PHONY: all clean build install lint format
.DEFAULT: all

all: build

format:
	clang-format -i $(SRC) $(HEADERS)

lint:
	$(LINTER) $(SRC) $(HEADERS) -- $(CFLAGS)

build: $(BUILD)/$(OUT)

$(BUILD)/$(OUT): $(OBJ)
	$(CC) -o $(BUILD)/$(OUT) $(LDFLAGS) $^

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CFLAG_ERRORS) -c -o $@ $<

clean:
	@rm -rf $(BUILD)

install:
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${BUILD}/${OUT} ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/${out}

test_buffer: test/test_buffer.c src/lib/buffer.c src/lib/next_pow_2.c src/lib/error.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(CFLAG_ERRORS) test/test_buffer.c src/lib/buffer.c src/lib/next_pow_2.c src/lib/error.c -o $(BUILD)/$@

test_ll: test/test_ll.c src/lib/linked_list.h
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(CFLAG_ERRORS) test/test_ll.c -o $(BUILD)/$@

test_hash_map: test/test_hash_map.c src/lib/hash_map.c src/lib/hash_map.h
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(CFLAG_ERRORS) test/test_hash_map.c src/lib/hash_map.c -o $(BUILD)/$@

.PHONY: test

test: test_buffer test_ll test_hash_map
	$(BUILD)/test_buffer
	$(BUILD)/test_ll
	$(BUILD)/test_hash_map