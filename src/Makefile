# the compiler: gcc for C program, define as g++ for C++
CC = g++

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g

# the build target executable:
TARGET = virtual-mem

$(TARGET): $(TARGET).cpp
	$(CC) -o $(TARGET) $(TARGET).cpp

clean:
	$(RM) $(TARGET)
