CC = $(shell which gcc)
CFLAGS = -Wall -I./include
LDFLAGS = -L ./lib -lftd3xx-static -lstdc++
EXAMPLE_TARGET=./bin/example
TARGET=

build: 
	${CC} -shared -o bin/libed.so -fPIC -I./include ./ed.c ./config.c ./utils.c -lm

.PHONY: build-example
build-example:
	$(CC) $(CFLAGS) $(LDFLAGS) -m64 -ffunction-sections -fmerge-all-constants -o $(EXAMPLE_TARGET)  ./utils.c ./utils.h ./example.c  $(LDFLAGS)

.PHONY: example
example: build-example

happy-path: ensure_bin
	${CC} -o bin/happy-path -I. -I./include test/happy-path.c ./utils.c ./ed.c ./config.c -lm -DED_DEBUG  $(LDFLAGS)

connect-tool: ensure_bin
	${CC} -o bin/connect-tool -I./include test/connect-tool.c src/ed.c src/config.c src/utils.c -lm -DED_DEBUG 

ensure_bin: 
	mkdir -p ./bin

help:
	@echo "make install - install the package"
	@echo "make example - install the package"
	@echo "make clean - remove all generated files"

default: help
