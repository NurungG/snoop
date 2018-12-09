CC=g++
CPPFLAGS=-g
OBJDIR=obj
TARGET=core_simulator
INCLUDES=-I ./include

vpath %.cc src
vpath %.h include

OBJS=$(addprefix $(OBJDIR)/, lru.o cache.o snoop.o main.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CPPFLAGS) -o $@ $^ $(INCLUDES)

$(OBJDIR)/%.o: %.cc
	$(CC) $(CPPFLAGS) -c -o $@ $< $(INCLUDES)

clean:
	rm -f $(OBJS) $(TARGET)
