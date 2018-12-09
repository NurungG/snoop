CC=g++
CPPFLAGS=-g
OBJS=snoop.o main.o
TARGET=core_simulator

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CPPFLAGS) -o $@ $^

clean:
	rm -f $(OBJS) $(TARGET)
