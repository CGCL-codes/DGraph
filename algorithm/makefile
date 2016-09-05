CC = g++#/usr/local/gcc-4.7.4/bin/g++4.7.4
CFLAGS += -fopenmp -std=c++0x -msse4 -w -O3 
LKFLAGS += 
LIBS = 
LD_DIR = #-Wl,-rpath,/usr/local/gcc-4.7.4/lib64/

INCLUDE_DIR = -I ../build/

INSTALL_DIR = ../../bin/

TARGET = pagerank bfs sssp k-core #rundemo lpa wcc qwcc

OBJS = ../build/mmap_file.o ../build/gfile.o ../build/sorter.o ../build/pathgraph.o

all:$(TARGET)
rundemo: rundemo.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) rundemo.o $(OBJS)  $(LIBS) -o $@

pagerank: pagerank.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) pagerank.o  $(OBJS) $(LIBS) -o $@

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

#Asyn App
asyn_pagerank: asyn_pagerank.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) asyn_pagerank.o  $(OBJS) $(LIBS) -o $@

%.o:%.cpp
	$(CC) $(CFLAGS)$(INCLUDE_DIR) $(LD_DIR) -c $< -o $@

#Normal sync compile
NORMAL_SYNC_TARGET = $(addsuffix _normal_sync, $(TARGET))

normal_sync: $(NORMAL_SYNC_TARGET)
%_normal_sync: %_normal_sync.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) $^  $(LIBS) -o $@

%_normal_sync.o: %.cpp
	$(CC) $(CFLAGS) -DSD_NORMAL_SYNC $(INCLUDE_DIR) $(LD_DIR) -c $< -o $@
	
normal_sync_install:
	make normal_sync
	cp $(NORMAL_SYNC_TARGET) $(INSTALL_DIR)


#sd_sync compile
SYNC_TARGET = $(addsuffix _sd_sync, $(TARGET))

sd_sync: $(SYNC_TARGET)
%_sd_sync: %_sd_sync.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) $^  $(LIBS) -o $@

%_sd_sync.o: %.cpp
	$(CC) $(CFLAGS) -DSD_SYNC $(INCLUDE_DIR) $(LD_DIR) -c $< -o $@
	
sd_sync_install:
	make sd_sync
	cp $(SYNC_TARGET) $(INSTALL_DIR)

#Normal async compile
NORMAL_ASYNC_TARGET = $(addsuffix _normal_async, $(TARGET))

normal_async: $(NORMAL_ASYNC_TARGET)
%_normal_async: %_normal_async.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) $^  $(LIBS) -o $@

%_normal_async.o: %.cpp
	$(CC) $(CFLAGS) -DSD_NORMAL_ASYNC $(INCLUDE_DIR) $(LD_DIR) -c $< -o $@
	
normal_async_install:
	make normal_async
	cp $(NORMAL_ASYNC_TARGET) $(INSTALL_DIR)

#sd async compile
ASYNC_TARGET = $(addsuffix _sd_async, $(TARGET))

sd_async: $(ASYNC_TARGET)
%_sd_async: %_sd_async.o $(OBJS)
	$(CC) $(CFLAGS) $(LKFLAGS) $(LD_DIR) $(INCLUDE_DIR) $^  $(LIBS) -o $@

%_sd_async.o: %.cpp
	$(CC) $(CFLAGS) -DSD_ASYNC $(INCLUDE_DIR) $(LD_DIR) -c $< -o $@
	
sd_async_install:
	make sd_async
	cp $(ASYNC_TARGET) $(INSTALL_DIR)

#All compare group
compare: $(NORMAL_SYNC_TARGET) $(SYNC_TARGET) $(NORMAL_ASYNC_TARGET) $(ASYNC_TARGET)

compare_install:
	make compare
	cp $(NORMAL_SYNC_TARGET) $(SYNC_TARGET) $(NORMAL_ASYNC_TARGET) $(ASYNC_TARGET) $(INSTALL_DIR)

install:
	cp $(TARGET) $(INSTALL_DIR) 

clean:
	rm -f *.o $(TARGET) $(NORMAL_SYNC_TARGET) $(SYNC_TARGET) $(NORMAL_ASYNC_TARGET) $(ASYNC_TARGET)



