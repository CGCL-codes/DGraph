CC = g++#/usr/local/gcc-4.7.4/bin/g++4.7.4
CFLAGS += -fopenmp -std=c++0x -msse4 -O3
LKFLAGS += 
LIBS = 
LD_DIR = #-Wl,-rpath,/usr/local/gcc-4.7.4/lib64/

INSTALL_DIR = ../../bin/

TARGET = build_normal build_scc_dag
OBJS = mmap_file.o gfile.o sorter.o pathgraph.o

all:$(TARGET)
build_normal: build_normal.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) build_normal.o $(OBJS) $(LIBS) -o $@

build_scc_dag: build_scc_dag.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) build_scc_dag.o $(OBJS) $(LIBS) -o $@

%pathgraph.o:%pathgraph.s 
	$(CC) $(CFLAGS) pathgraph.s $(LD_DIR) -c $< -o $@

%.o:%.cpp
	$(CC) $(CFLAGS) $(LD_DIR) -c $< -o $@

install:
	cp $(TARGET) $(INSTALL_DIR) 

clean:
	rm -f *.o $(TARGET) 
