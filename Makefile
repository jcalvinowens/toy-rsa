CC = gcc
CFLAGS = -O3 -fno-strict-aliasing -D_GNU_SOURCE -Wall -Wextra \
	-Werror=implicit -Wdeclaration-after-statement -Wstrict-prototypes \
	-Wmissing-prototypes -Wmissing-declarations

debug: CFLAGS += -Og -g -fsanitize=address -fsanitize=undefined

obj = bfi.o rng.o rsa.o main.o
binary = toy-rsa

all: $(obj) $(binary)
debug: all

disasm: CFLAGS += -fverbose-asm
disasm: $(obj:.o=.s)

$(binary): $(obj)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@

%.s: %.c
	$(CC) $< $(CFLAGS) -c -S -o $@

clean:
	rm -f *.s *.o toy-rsa
