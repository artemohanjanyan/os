all: rshd

rshd: rshd.c
	g++ -std=c++11 rshd.c -o rshd -Wall -Wextra -pedantic -O2 -Wshadow -Wformat=2 -Wfloat-equal -Wconversion -Wlogical-op -Wcast-qual -Wcast-align -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fstack-protector -lmcheck -D_FORTIFY_SOURCE=2 

clean:
	$(RM) rshd
