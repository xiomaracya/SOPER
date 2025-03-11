TARGET = voting

CC = gcc

CFLAGS = -Wall -Wextra -g

SRCS = principal.c

OBJS = principal.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
