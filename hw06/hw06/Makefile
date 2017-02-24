
BINS   := list_sum_ints array_sum_ints random_sizes
CFLAGS := -g

all: $(BINS)

list_sum_ints: list_sum_ints.o nu_mem.o

array_sum_ints: array_sum_ints.o nu_mem.o

random_sizes: random_sizes.o nu_mem.o trand.o

%.o : %.c $(wildcard *.h)

clean:
	rm -f $(BINS) *.o stats.tmp out.tmp

test:
	perl test.t

.PHONY: clean all test
