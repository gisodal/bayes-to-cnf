DIR  = $(shell cd "$( dirname "$0" )" && pwd)
SRC = $(DIR)/src

.PHONY: all clean debug build install

build: all

all:
	make -C $(SRC) all

debug:
	make -C $(SRC) debug

install:
	make -C $(SRC) install

clean:
	make -C $(SRC) clean
	rm -rf bin include lib*
