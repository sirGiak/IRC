CC=gcc
CFLAGS=-Wall -g
TARGET = ircd
OBJS = objs/server.o objs/utils_server.o objs/service_server.o objs/utils_service_server.o
TARGET_CLIENT = irc
OBJS_CLIENT = objs/client.o objs/utils_client.o
VERS_CLIENT = 1.0
VERS_SERVER = 1.0
PACKAGE_SERVER = $(TARGET)-$(VERS_SERVER)
PACKAGE_CLIENT = $(TARGET_CLIENT)-$(VERS_CLIENT)

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

package-server:
	#eseguire come root
	sudo mkdir -p $(PACKAGE_SERVER)/usr/bin
	
	sudo cp $(TARGET) $(PACKAGE_SERVER)/usr/bin/$(TARGET)
	sudo chown root:root $(PACKAGE_SERVER)/usr/bin/$(TARGET)
	sudo mkdir -p $(PACKAGE_SERVER)/DEBIAN
	
	echo "Package: $(PACKAGE_SERVER)\nArchitecture: all\nVersion: $(VERS_SERVER)\nSection: games\nMaintainer: Local user <root@localhost>\nPriority: optional\nStandards-Version: 4.7.0\nDescription: SERVER IRC\n" > $(PACKAGE_SERVER)/DEBIAN/control
	
	
	dpkg-deb -b $(PACKAGE_SERVER)
	
package-client:
	#eseguire come root
	sudo mkdir -p $(PACKAGE_CLIENT)/usr/bin
	
	sudo cp $(TARGET_CLIENT) $(PACKAGE_CLIENT)/usr/bin/$(TARGET_CLIENT)
	sudo chown root:root $(PACKAGE_CLIENT)/usr/bin/$(TARGET_CLIENT)
	sudo mkdir -p $(PACKAGE_CLIENT)/DEBIAN
	
	echo "Package: $(PACKAGE_CLIENT)\nArchitecture: all\nVersion: $(VERS_CLIENT)\nSection: games\nMaintainer: Local user <root@localhost>\nPriority: optional\nStandards-Version: 4.7.0\nDescription: Client IRC\n" > $(PACKAGE_CLIENT)/DEBIAN/control
	
	
	dpkg-deb -b $(PACKAGE_CLIENT)
	


.PHONY: all clear package-client package-server
