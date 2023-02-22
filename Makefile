CC=gcc
CFLAGS=-O2 -Wall -Wextra -pedantic -std=c99

.PHONY: all
all: float_test.exe float_test2.exe

%.exe: %.o float_calc.o
	$(CC) -o $@ $^

%o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

.PHONY: clean
clean:
	rm *.o *.exe | cat
