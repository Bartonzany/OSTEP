BINARIES = leak overflow useafterfree null vector

CC = gcc
CFLAGS = -g -Wall

all: $(BINARIES)

clean:
	rm -f $(BINARIES)

leak: leak.c
	$(CC) $(CFLAGS) -o leak leak.c

overflow: overflow.c
	$(CC) $(CFLAGS) -o overflow overflow.c

useafterfree: useafterfree.c
	$(CC) $(CFLAGS) -o useafterfree useafterfree.c

null: null.c
	$(CC) $(CFLAGS) -o null null.c

vector: vector.c
	$(CC) $(CFLAGS) -o vector vector.c