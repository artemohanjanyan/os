all: main

main.o: main.c
	gcc -c main.c -o main.o

hello1.o: hello1.c
	gcc -c hello1.c -o hello1.o
libhello1.a: hello1.o
	ar rcs libhello1.a hello1.o

hello2.o: hello2.c
	gcc -fPIC hello2.c -c -o hello2.o
libhello2.so: hello2.o
	gcc -shared hello2.o -o libhello2.so

main: main.o libhello1.a libhello2.so
	gcc -s main.o -L. -lhello1 -lhello2 -o main

clean:
	rm *.o *.a *.so main
