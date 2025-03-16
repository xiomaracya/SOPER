TARGET = voting

CC = gcc

CFLAGS = -Wall -Wextra -g -lpthread

SRCS = principal.c votante.c signal_handles.c

OBJS = principal.o votante.o signal_handles.c

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
