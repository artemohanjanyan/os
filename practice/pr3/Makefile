ARGS=-std=c11 -Wall -Wextra -pedantic -Wformat=2 -Wfloat-equal -Wconversion -Wlogical-op -Wcast-qual -Wcast-align -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=address -fstack-protector -lmcheck -D_FORTIFY_SOURCE=2

all: cat_prep

cat_prep.o: cat_prep.c
	gcc $(ARGS) -c cat_prep.c -o cat_prep.o

cat_prep: cat_prep.o
	gcc $(ARGS) -s cat_prep.o -o cat_prep

clean:
	$(RM) cat_prep cat_prep.o
