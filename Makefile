CC=gcc
CFLAGS=-Wall -g
TARGET = server
OBJS = objs/server.o objs/utils_server.o objs/service_server.o objs/utils_service_server.o
TARGET_CLIENT = client
OBJS_CLIENT = objs/client.o objs/utils_client.o

all: $(TARGET) $(TARGET_CLIENT)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET_CLIENT): $(OBJS_CLIENT)
	$(CC) $(CFLAGS) -o $@ $^

objs/%.o:%.c
	mkdir -p objs
	$(CC) $(CFLAGS) -o $@ -c $<

clear:
	rm objs/*.o

.PHONY: all clear
