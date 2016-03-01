all: cat

cat.o: cat.c
	gcc -c cat.c -o cat.o

cat: cat.o
	gcc -s cat.o -o cat

clean:
	$(RM) cat cat.o
