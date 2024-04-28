brainfuck: main.c
	${CC} ${CFLAGS} ${LDFLAGS} main.c -o brainfuck

clean:
	rm brainfuck
