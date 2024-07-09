CC=gcc
CFLAGS=-Wall -O3 -g
LDFLAGS= -pthread

bin/drama-ng: build/main.o build/sender.o build/receiver.o build/helper.o \
	build/config.o
	$(CC) $(LDFLAGS) -o $@ $^

build/%.o: src/%.c src/%.h src/asm.h
	$(CC) $(CFLAGS) -c -o $@ $<

build/main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	-rm build/*

.PHONY: cleanall
cleanall: clean
	-rm bin/*
