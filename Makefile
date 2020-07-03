
# The compiler: gcc for C programs
CC = gcc

# Compiler flags:
# -Wall for debugger warnings
CFLAGS = -Wall -Wextra -Werror
ifdef DEBUG
	CFLAGS += -g
endif

# The name of the program that we are producing.
TARGET = my_tar

# When we just run "make", what gets built? 
# This is a "phony" target
# that just tells make what other targets to build.
all: $(TARGET)

# All the .o files we need for our executable.
OBJS = my_tar.o functions.o

# The executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# An option for making a debug target
debug: CFLAGS += -g
debug: $(TARGET)

# Individual source files
my_tar.o: my_tar.c my_tar.h
	$(CC) $(CFLAGS) -c my_tar.c
functions.o: functions.c my_tar.h
	$(CC) $(CFLAGS) -c functions.c

# A "phony" target to remove built files and backups
clean:
	rm -f *.o $(TARGET) *~