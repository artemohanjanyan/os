man 2 (open
		|read
		|write
		|close)

int open(char *path, int flags);
freopen("file", "rw");

read и write могут прочитать/записать не полностью. Потому что буферы.
