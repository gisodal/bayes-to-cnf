DIR  = $(shell cd "$( dirname "$0" )" && pwd)
BNTCNF=$(DIR)/bn-to-cnf
QM=$(DIR)/quine-mccluskey

.PHONY: all clean debug build install

build: all

all:
	make -C $(QM) static build
	make -C $(QM) install-static install-include install-bin
	make -C $(BNTCNF) static build
	make -C $(BNTCNF) install-static install-include install-bin

debug:
	make -C $(QM) debug-static debug
	make -C $(QM) install-static install-include install-bin
	make -C $(BNTCNF) debug-static debug
	make -C $(BNTCNF) install-static install-include install-bin

install:
	make -C $(QM) install-static install-include install-bin
	make -C $(BNTCNF) install-static install-include install-bin

clean:
	make -C $(QM) clean
	make -C $(BNTCNF) clean

