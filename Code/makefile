override CFLAGS := -std=gnu99 -O0 -g $(CFLAGS)

fs.o: fs.c fs.h disk.c disk.h

.PHONY: clean

clean:
	rm -f fs.o