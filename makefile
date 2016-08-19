CC = /usr/local/gcc-4.7.4/bin/g++4.7.4
CFLAGS += -fopenmp -std=c++0x 
LKFLAGS += 
LIBS = 
LD_DIR = -Wl,-rpath,/usr/local/gcc-4.7.4/lib64/

INSTALL_DIR=../bin/

SUB_DIR1 = build
SUB_DIR2 = algorithm

SUB_DIR = $(SUB_DIR1) $(SUB_DIR2)

all: CHECK_DIR $(SUB_DIR)
CHECK_DIR: 
	mkdir -p $(INSTALL_DIR)
ECHO:
	@echo $(SUB_DIR)

$(SUB_DIR): ECHO
	make -j16 -C $@

install:
	make
	make install -s -C $(SUB_DIR1)
	make install -s -C $(SUB_DIR2)

clean:
	make clean -s -C $(SUB_DIR1)
	make clean -s -C $(SUB_DIR2)
