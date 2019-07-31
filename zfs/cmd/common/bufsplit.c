#include <stdlib.h>
#include <string.h>
#include "bufsplit.h"

static char *bsplitchar = "\t\n";

size_t
bufsplit(char *buf, size_t dim, char **array)
{
	unsigned numsplit;
	int i;

	if (!buf)
		return (0);
	if (!dim ^ !array)
		return (0);
	if (buf && !dim && !array) {
		bsplitchar = buf;
		return (1);
	}
	numsplit = 0;
	while (numsplit < dim) {
		array[numsplit] = buf;
		numsplit++;
		buf = strpbrk(buf, bsplitchar);
		if (buf)
			*(buf++) = '\0';
		else
			break;
		if (*buf == '\0') {
			break;
		}
	}
	buf = strrchr(array[numsplit-1], '\0');
	for (i = numsplit; i < dim; i++)
		array[i] = buf;
	return (numsplit);
}