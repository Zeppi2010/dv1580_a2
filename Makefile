# Compiler and flags
CC = g++
CFLAGS = -Wall -fPIC -pthread

# Source files
MEMORY_MANAGER_SRC = memory_manager.c
LINKED_LIST_SRC = linked_list.c

# Object files
MEMORY_MANAGER_OBJ = memory_manager.o
LINKED_LIST_OBJ = linked_list.o

# Library name based on the OS
ifeq ($(OS),Windows_NT)
    LIBRARY_NAME = libmemory_manager.dll
    CLEAN_CMD = del /q
else
    LIBRARY_NAME = libmemory_manager.so
    CLEAN_CMD = rm -f
endif

# Targets
all: mmanager list

# Build the memory manager dynamic library
mmanager: $(MEMORY_MANAGER_OBJ)
	$(CC) -shared -o $(LIBRARY_NAME) $(MEMORY_MANAGER_OBJ)
	@echo "Compiled memory manager as a dynamic library: $(LIBRARY_NAME)"

# Build the linked list application
list: $(LINKED_LIST_OBJ)
	$(CC) $(CFLAGS) -o linked_list_app $(LINKED_LIST_OBJ) -L. -l:$(LIBRARY_NAME)
	@echo "Compiled linked list application: linked_list_app"

# Compile memory manager source file
$(MEMORY_MANAGER_OBJ): $(MEMORY_MANAGER_SRC)
	$(CC) $(CFLAGS) -c $(MEMORY_MANAGER_SRC) -o $(MEMORY_MANAGER_OBJ)
	@echo "Compiled: $(MEMORY_MANAGER_SRC)"

# Compile linked list source file
$(LINKED_LIST_OBJ): $(LINKED_LIST_SRC)
	$(CC) $(CFLAGS) -c $(LINKED_LIST_SRC) -o $(LINKED_LIST_OBJ)
	@echo "Compiled: $(LINKED_LIST_SRC)"

# Clean up
clean:
	$(CLEAN_CMD) $(MEMORY_MANAGER_OBJ) $(LINKED_LIST_OBJ) $(LIBRARY_NAME) linked_list_app
	@echo "Cleaned up project files"

.PHONY: all mmanager list clean
