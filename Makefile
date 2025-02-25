TARGET = main

CC = gcc

CFLAGS = -Wall -Wextra -g

SRCS = main.c minero.c monitor.c pow.c

OBJS = main.o minero.o monitor.o pow.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

distclean: clean
	rm -f *~