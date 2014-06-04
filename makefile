OBJS = hashmap.o order.o sorted-list.o thread.o tokenizer.o 
CC = gcc
CFLAGS = -g -Wall -pthread

thread: $(OBJS) 
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f thread *.o
