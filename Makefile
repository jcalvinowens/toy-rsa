CC = gcc
CFLAGS = -O2 -fno-strict-aliasing -D_GNU_SOURCE -Wall -Wextra \
	-Werror=implicit -Wdeclaration-after-statement -Wstrict-prototypes \
	-Wmissing-prototypes -Wmissing-declarations

debug: CFLAGS += -Og -g -fsanitize=address -fsanitize=undefined
debug: LDFLAGS := $(LDFLAGS) -lasan -lubsan

obj = bfi.o rng.o rsa.o main.o
binary = toy-rsa

all: $(obj) $(binary)
debug: all

$(binary): $(obj)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@

clean:
	rm -f *.o toy-rsa
