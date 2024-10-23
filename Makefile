# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -fPIC -pthread
LIBS = -L. -lmemory_manager -lpthread

# Targets
.PHONY: all clean mmanager list

all: mmanager list

mmanager: memory_manager.o
	$(CC) -shared -o memory_manager.dll memory_manager.o -Wl,--out-implib=libmemory_manager.a

list: linked_list.o
	@echo "Linked list compiled, but not generating an executable."

# Compilation
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	del /Q memory_manager.o linked_list.o memory_manager.dll libmemory_manager.a
