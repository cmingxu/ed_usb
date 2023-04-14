CC = $(shell which gcc)
CFLAGS = -Wall -I./include
LDFLAGS = -L ./lib -lftd3xx-static -lstdc++
EXAMPLE_TARGET=./bin/example
TARGET=

build: 
	echo "building"

.PHONY: build-example
build-example:
	$(CC) $(CFLAGS) $(LDFLAGS) -m64 -ffunction-sections -fmerge-all-constants -o $(EXAMPLE_TARGET)  ./example.c $(LDFLAGS)

.PHONY: example
example: build-example

help:
	@echo "make install - install the package"
	@echo "make example - install the package"
	@echo "make clean - remove all generated files"

default: help
