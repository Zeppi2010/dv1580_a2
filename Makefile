# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -fPIC -pthread
LIBS = -L. -lmemory_manager -lpthread

# Targets
.PHONY: all clean mmanager list test

all: mmanager list

mmanager: memory_manager.o
	$(CC) -shared -o memory_manager.dll memory_manager.o -Wl,--out-implib=libmemory_manager.a

list: linked_list.o
	@echo "Linked list compiled, but not generating an executable."

test: test_linked_list.o
	$(CC) -o test_linked_list test_linked_list.o $(LIBS)

# Compilation
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f memory_manager.o linked_list.o memory_manager.dll libmemory_manager.a test_linked_list.o test_linked_list
