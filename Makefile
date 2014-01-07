CC = gcc
INCLUDE = 
BIN = 
LIB = 

#CFLAGS = -D_GNU_SOURCE -Wall -g
CFLAGS =  -D_GNU_SOURCE -Wall -O2 
LDFLAGS = 

TARGET = ae-main

all: $(TARGET)

ae-main: ae_main.o ae.o
	$(CC) -o $@ $^ $(LIB) 

%.o : %.c	
	$(CC) -c $(CFLAGS) $< $(INCLUDE)

clean :
	$(RM) $(TARGET) *.o

   

