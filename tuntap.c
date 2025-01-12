#include "tuntap.h"

static void read_write(int in, int out)
{
	char buffer[BUFFLEN];
	ssize_t count;

	count = read(in, buffer, BUFFLEN);
	if (count > 0) {
		// show_buffer(buffer, count);
		write(out, buffer, count);
	}
}

int main(int argc, char** argv)
{
	int fd0;
	int fd1;

	fd0 = open_tuntap("tun0", O_NONBLOCK);
	if (!fd0)
		return 1;

	fd1 = open_tuntap("tun1", O_NONBLOCK);
	if (!fd1)
		return 1;

	if (tuntap_system())
		return 1;

	if (argc == 1) {
		if(daemon(0, 1))
			return 1;
	}

	while (1) {

		read_write(fd0, fd1);
		read_write(fd1, fd0);
	}

    return 0;
}
