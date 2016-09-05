CC =  g++#/usr/local/gcc-4.7.4/bin/g++4.7.4
CFLAGS += -fopenmp -std=c++0x -msse4 -O3 
LKFLAGS += 
LIBS = 
LD_DIR = #-Wl,-rpath,/usr/local/gcc-4.7.4/lib64/

INCLUDE_DIR = -I ../build/

INSTALL_DIR = ../../bin/


TARGET = analysis i_to_vid convert_adjlist_to_edgelist #get_topo_top
OBJS = ../build/mmap_file.o ../build/gfile.o ../build/sorter.o ../build/pathgraph.o

all:$(TARGET)
analysis: analysis.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) analysis.o $(OBJS)  $(LIBS) -o $@

i_to_vid: i_to_vid.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) i_to_vid.o $(OBJS)  $(LIBS) -o $@

convert_adjlist_to_edgelist: convert_adjlist_to_edgelist.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) convert_adjlist_to_edgelist.o $(OBJS)  $(LIBS) -o $@

get_topo_top: get_topo_top.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) get_topo_top.o $(OBJS)  $(LIBS) -o $@
%.o:%.cpp
	$(CC) $(CFLAGS)$(INCLUDE_DIR) $(LD_DIR) -c $< -o $@

install:
	cp $(TARGET) $(INSTALL_DIR) 

clean:
	rm -f *.o $(TARGET) 
