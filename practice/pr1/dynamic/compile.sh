gcc -c main.c -o main.o
gcc -c hello.c -o hello.o

gcc -fPIC hello.c -c -o hello.o
gcc -shared hello.o -o libhello.so
gcc -s main.o -o main -L. -lhello
