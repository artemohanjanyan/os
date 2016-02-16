gcc -c hello.c -o hello.o
gcc -c main.c -o main.o

ar rcs libhello.a hello.o

gcc -static main.o -L. -lhello -o main
