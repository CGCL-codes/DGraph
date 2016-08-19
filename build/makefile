CC = /usr/local/gcc-4.7.4/bin/g++4.7.4
CFLAGS += -fopenmp -std=c++0x -msse4 
LKFLAGS += 
LIBS = 
LD_DIR = -Wl,-rpath,/usr/local/gcc-4.7.4/lib64/

INSTALL_DIR = ../../bin/

TARGET = build analysis build_match_group 
OBJS = mmap_file.o gfile.o sorter.o pathgraph.o

all:$(TARGET)
build: build.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) build.o $(OBJS) $(LIBS) -o $@

analysis: analysis.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) analysis.o $(OBJS) $(LIBS) -o $@

build_match_group: build_match_group.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) build_match_group.o $(OBJS) $(LIBS) -o $@

%pathgraph.o:%pathgraph.s 
	$(CC) $(CFLAGS) pathgraph.s $(LD_DIR) -c $< -o $@

%.o:%.cpp
	$(CC) $(CFLAGS) $(LD_DIR) -c $< -o $@

install:
	cp $(TARGET) $(INSTALL_DIR) 

clean:
	rm -f *.o $(TARGET) 
