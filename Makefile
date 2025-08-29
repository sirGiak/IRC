CC=gcc
CFLAGS=-Wall -g
TARGET = main
OBJS = objs/main.o objs/network.o
TARGET_HOST = host
# todo SET ALGORITMO, commentando gli altri due
#OBJS_HOST = objs/host.o objs/flooding.o
OBJS_HOST = objs/host.o objs/floodmax.o
#OBJS_HOST = objs/host.o objs/bellman_ford.o #todo cambiare valore di M_SIZE e il file in lettura per la matrice

all: $(TARGET) $(TARGET_HOST)
	rm -rf fifo

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET_HOST): $(OBJS_HOST)
	$(CC) $(CFLAGS) -o $@ $^

objs/%.o:%.c
	$(CC) $(CFLAGS) -o $@ -c $<

clear:
	rm objs/*.o

.PHONY: all clear
