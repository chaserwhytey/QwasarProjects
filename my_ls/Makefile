
# The compiler: gcc for C programs
CC = gcc

# Compiler flags:
# -Wall for debugger warnings
CFLAGS = -Wall -Wextra -Werror
ifdef DEBUG
	CFLAGS += -g
endif

# The name of the program that we are producing.
TARGET = my_ls

# When we just run "make", what gets built? 
# This is a "phony" target
# that just tells make what other targets to build.
all: $(TARGET)

# All the .o files we need for our executable.
OBJS = my_ls.o main.o

# The executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# An option for making a debug target
debug: CFLAGS += -g
debug: $(TARGET)

# Individual source files
my_ls.o: my_ls.c my_ls.h
	$(CC) $(CFLAGS) -c my_ls.c
main.o: main.c my_ls.h
	$(CC) $(CFLAGS) -c main.c

# A "phony" target to remove built files and backups
clean:
	rm -f *.o my_ls *~