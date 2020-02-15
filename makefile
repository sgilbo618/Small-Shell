
smallsh.o: smallsh.c
	gcc -std=gnu99 -c smallsh.c -o smallsh.o

smallsh: smallsh.o
	gcc -std=gnu99 smallsh.o -o smallsh

clean:
	-rm -f smallsh.o smallsh

