area:
	gcc -o area area.s

area.s: area.c
	gcc -S -O0 -o area.s area.c

area-O2.s: area.c
	gcc -S -O2 -o area-O2.s area.c

clean:
	rm -f area *.s

.PHONY: clean
