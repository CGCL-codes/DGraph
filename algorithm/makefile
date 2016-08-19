CC = /usr/local/gcc-4.7.4/bin/g++4.7.4
CFLAGS += -fopenmp -std=c++0x -msse4 
LKFLAGS += 
LIBS = 
LD_DIR = -Wl,-rpath,/usr/local/gcc-4.7.4/lib64/

INCLUDE_DIR = -I ../build/

INSTALL_DIR = ../../bin/


TARGET = rundemo pagerank lpa bfs wcc sssp qwcc k-core pr
OBJS = ../build/mmap_file.o ../build/gfile.o ../build/sorter.o ../build/pathgraph.o

all:$(TARGET)
rundemo: rundemo.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) rundemo.o $(OBJS)  $(LIBS) -o $@

pagerank: pagerank.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) pagerank.o $(OBJS) $(LIBS) -o $@

pr: pr.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) pr.o $(OBJS) $(LIBS) -o $@

lpa : lpa.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) lpa.o $(OBJS) $(LIBS) -o $@

sssp : sssp.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) sssp.o $(OBJS) $(LIBS) -o $@

bfs : bfs.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) bfs.o $(OBJS) $(LIBS) -o $@

wcc : wcc.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) wcc.o $(OBJS) $(LIBS) -o $@

qwcc : qwcc.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) qwcc.o $(OBJS) $(LIBS) -o $@

k-core : k-core.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) k-core.o $(OBJS) $(LIBS) -o $@

%.o:%.cpp
	$(CC) $(CFLAGS)$(INCLUDE_DIR) $(LD_DIR) -c $< -o $@

install:
	cp $(TARGET) $(INSTALL_DIR) 

clean:
	rm -f *.o $(TARGET) 
