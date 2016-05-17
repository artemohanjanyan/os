OPTIONS=-g -Wall -Wextra -pedantic -O2 -Wshadow -Wformat=2 -Wfloat-equal -Wconversion -Wlogical-op -Wcast-qual -Wcast-align -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=address -fstack-protector -lmcheck -D_FORTIFY_SOURCE=2
# -fsanitize=address

all: rshd.o string_buffer.o
	g++ $(OPTIONS) rshd.o string_buffer.o -o rshd

rshd.o: rshd.c string_buffer.h
	gcc $(OPTIONS) rshd.c -c -o rshd.o

string_buffer.o: string_buffer.h string_buffer.cpp
	g++ -std=c++11 $(OPTIONS) -c string_buffer.cpp -o string_buffer.o

clean:
	$(RM) rshd rshd.o string_buffer.o
