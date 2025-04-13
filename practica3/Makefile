TARGETS = miner monitor

CC = gcc

CFLAGS = -Wall -Wextra -g -lpthread -lrt

MINER_SRCS = minero.c pow.c
MONITOR_SRCS = monitor.c pow.c

MINER_OBJS = $(MINER_SRCS:.c=.o)
MONITOR_OBJS = $(MONITOR_SRCS:.c=.o)

all: $(TARGETS)

miner: $(MINER_OBJS)
	$(CC) $(CFLAGS) -o miner $(MINER_OBJS)

monitor: $(MONITOR_OBJS)
	$(CC) $(CFLAGS) -o monitor $(MONITOR_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o miner monitor
