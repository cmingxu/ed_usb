CC = $(shell which gcc)
CFLAGS = -Wall -I./include
LDFLAGS = -L ./lib -lftd3xx-static -lstdc++
EXAMPLE_TARGET=./bin/example
TARGET=

release: build-static
	mkdir -p ./release
	cp -r ./include ./release
	cp ./ed.h ./release
	cp ./utils.h ./release
	cp ./bin/libed-static.a ./release
	cp ./lib/libftd3xx-static.a ./release
	tar cvf release.tar ./release
	rm -r ./release

build-static:
	${CC} -c   -I./include ./ed.h ./ed.c -lm
	${CC} -c   -I./include ./ed.h ./config.c -lm
	${CC} -c   -I./include ./utils.h ./utils.c -lm
	ar cr bin/libed-static.a ed.o config.o utils.o
	rm *.o

.PHONY: build-example
build-example:
	$(CC) $(CFLAGS) $(LDFLAGS) -m64 -ffunction-sections -fmerge-all-constants -o $(EXAMPLE_TARGET)  ./utils.c ./utils.h ./example.c  $(LDFLAGS)

.PHONY: example
example: build-example

happy-path: ensure_bin
	${CC} -o bin/happy-path -I. -I./include test/happy-path.c ./utils.c ./ed.c ./config.c -lm -DED_DEBUG  $(LDFLAGS)

happy-path-with-shared-object: ensure_bin
	${CC} -o bin/happy-path-with-so -I. -I./include test/happy-path.c -L ./bin -led -lm -DED_DEBUG  $(LDFLAGS)

happy-path-with-static-object: ensure_bin
	${CC} -o bin/happy-path-with-a -I. -I./include test/happy-path.c bin/libed-static.a -lm -DED_DEBUG  $(LDFLAGS)

with-serial-no: ensure_bin
	${CC} -o bin/create_with_serial_no -I. -I./include test/create_handle_with_serial_no.c ./utils.c ./ed.c ./config.c -lm -DED_DEBUG  $(LDFLAGS)
reset-601: ensure_bin
	${CC} -o bin/reset601 -I. -I./include test/reset-device-with601mode.c ./utils.c ./ed.c ./config.c -lm -DED_DEBUG  $(LDFLAGS)

reset-null: ensure_bin
	${CC} -o bin/resetnull -I. -I./include test/reset-device-null.c ./utils.c ./ed.c ./config.c -lm -DED_DEBUG  $(LDFLAGS)

reset-245: ensure_bin
	${CC} -o bin/reset245 -I. -I./include test/reset-device-with-245.c ./utils.c ./ed.c ./config.c -lm -DED_DEBUG  $(LDFLAGS)

reset-default: ensure_bin
	${CC} -o bin/resetdefault -I. -I./include test/reset-default.c ./utils.c ./ed.c ./config.c -lm -DED_DEBUG  $(LDFLAGS)

get_chip_config: ensure_bin
	${CC} -o bin/get_chip_config -I. -I./include test/get_device_config.c ./utils.c ./ed.c ./config.c -lm -DED_DEBUG  $(LDFLAGS)

get_queue_status: ensure_bin
	${CC} -o bin/get_queue_status -I. -I./include test/get_queue_status.c ./utils.c ./ed.c ./config.c -lm -DED_DEBUG  $(LDFLAGS)

abort_pipe: ensure_bin
	${CC} -o bin/abort_pipe -I. -I./include test/abort_pipe.c ./utils.c ./ed.c ./config.c -lm -DED_DEBUG  $(LDFLAGS)

get_info: ensure_bin
	${CC} -o bin/get_info -I. -I./include test/get_info.c ./utils.c ./ed.c ./config.c -lm -DED_DEBUG  $(LDFLAGS)

connect-tool: ensure_bin
	${CC} -o bin/connect-tool -I./include test/connect-tool.c src/ed.c src/config.c src/utils.c -lm -DED_DEBUG 

ensure_bin: 
	mkdir -p ./bin

help:
	@echo "make install - install the package"
	@echo "make example - install the package"
	@echo "make clean - remove all generated files"

default: help
