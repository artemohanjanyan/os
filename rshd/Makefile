all: rshd

rshd: rshd.c
	gcc rshd.c -o rshd -Wall -Wextra -pedantic -O2 -Wshadow -Wformat=2 -Wfloat-equal -Wconversion -Wlogical-op -Wcast-qual -Wcast-align -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=address -fstack-protector -lmcheck -D_FORTIFY_SOURCE=2
	# -fsanitize=address

clean:
	$(RM) rshd
