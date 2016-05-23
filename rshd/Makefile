CC=gcc
CCPP=g++ -std=c++11
OPTIONS=-g -Wall -Wextra -pedantic -O2 -Wshadow -Wformat=2 -Wfloat-equal -Wconversion -Wlogical-op -Wcast-qual -Wcast-align -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=address -fstack-protector -lmcheck -D_FORTIFY_SOURCE=2
OPTIONS_R=-O2
# -fsanitize=address

all: rshd

rshd: rshd.o string_buffer.o ptr_set.o
	$(CCPP) $(OPTIONS) rshd.o string_buffer.o ptr_set.o -o rshd

rshd.o: rshd.c string_buffer.h
	$(CC) $(OPTIONS) -c rshd.c -o rshd.o

string_buffer.o: string_buffer.h string_buffer.cpp
	$(CCPP) $(OPTIONS) -c string_buffer.cpp -o string_buffer.o

ptr_set.o: ptr_set.h ptr_set.cpp
	$(CCPP) $(OPTIONS) -c ptr_set.cpp -o ptr_set.o

clean:
	$(RM) rshd rshd.o string_buffer.o ptr_set.o
