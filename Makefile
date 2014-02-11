CC = gcc
INCLUDE = -I${JUMBO_ROOT}/include
BIN = 
LIB = -L${JUMBO_ROOT}/lib -lhiredis

#CFLAGS = -D_GNU_SOURCE -Wall -g
CFLAGS = -Wall -O2 
LDFLAGS = 

TARGET = ae-main

all: $(TARGET)

ae-main: ae_main.o ae.o anet.o
	$(CC) -o $@ $^ $(LIB) 

benchmark: benchmark.o ae.o anet.o sds.o adlist.o
	$(CC) -o $@ $^ $(LIB) 

%.o : %.c	
	$(CC) -c $(CFLAGS) $< $(INCLUDE)

clean :
	$(RM) $(TARGET) benchmark *.o

   

