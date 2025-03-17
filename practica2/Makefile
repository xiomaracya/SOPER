TARGET = voting

CC = gcc

CFLAGS = -Wall -Wextra -g -lpthread

SRCS = principal.c votante.c

OBJS = principal.o votante.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
