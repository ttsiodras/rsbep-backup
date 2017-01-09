#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static char buf[131072];

int main(int argc, char *argv[])
{
    if (argc != 2) {
	fprintf(stderr, "Usage: %s size\n\twill read size bytes from stdin "
		"and print them on stdout\n", argv[0]);
	exit(1);
    }
    long long total = atoll(argv[1]);
    if (total) {
	long long sofar = 0;
	while(sofar<total) {
	    int len = read(0, buf, 131072);
	    if (!len) break;
	    if (sofar + len <= total) {
		if (len != write(1, buf, len)) {
		    perror("Write failed!");
		    exit(1);
		}
		sofar += len;
	    } else {
		if (total-sofar != write(1, buf, total-sofar)) {
		    perror("Write failed!");
		    exit(1);
		}
		sofar = total;
	    }
	}
    }
    return 0;
}
