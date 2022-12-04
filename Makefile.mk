CC	= gcc
CFLAGS	= -g -Wall -Wvla -fsanitize=address

all: mymalloc.c test.c
	$(CC) $(CFLAGS) -o test $^

%: %.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(TARGET) *.o *.a *.dylib *.dSYM
