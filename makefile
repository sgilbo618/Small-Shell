# Name: Samantha Guilbeault
# Date: 2/16/2020
# Descriptions: Makefile for program03 - smallsh

CFLAGS= -std=gnu99


dynArr.o: dynamicArray.c dynArray.h
	gcc -c dynamicArray.c -o dynArr.o $(CFLAGS)

smallsh.o: smallsh.c dynArr.o
	gcc -c smallsh.c -o smallsh.o $(CFLAGS)

smallsh: smallsh.o dynArr.o
	gcc smallsh.o dynArr.o -o smallsh $(CFLAGS)

clean:
	-rm -f dynArr.o smallsh.o smallsh

