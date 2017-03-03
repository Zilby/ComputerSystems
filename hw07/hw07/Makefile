
BINS   := sum_sys sum_hw06 sum_opt
CFLAGS := -g -O2

all: $(BINS)

sum_sys: list_sum_ints.o sys_mem.o
	$(CC) $(CFLAGS) -o sum_sys list_sum_ints.o sys_mem.o

sum_hw06: list_sum_ints.o nu_mem.o hw06_mem.o
	$(CC) $(CFLAGS) -o sum_hw06 list_sum_ints.o nu_mem.o hw06_mem.o

sum_opt: list_sum_ints.o opt_mem.o hw06_mem.o
	$(CC) $(CFLAGS) -o sum_opt list_sum_ints.o opt_mem.o hw06_mem.o

%.o: %.c $(wildcard *.h)

clean:
	rm -f $(BINS) *.o *.tmp

test:
	perl test.t

.PHONY: clean all test
