#include "stdio.h"

int main()
{
	char buf[1024];
	int length;
	int i;
	while (1)
	{
		length = read(0, buf, 1024);
		if (length == -1)
		{
			write(2, "read error\n", 11);
			return 0;
		}

		if (length == 0)
			return 0;

		i = 0;
		while (i < length)
		{
			int ret = write(1, buf + i, length - i);
			if (ret == -1)
			{
				write(2, "write error\n", 12);
				return 0;
			}

			i += ret;
		}
	}

	return 0;
}
