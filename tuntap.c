#include "tuntap.h"
#include <time.h>
#include <sys/epoll.h>

struct buff {
	ssize_t count;
	char buffer[BUFFLEN];
};

struct buff buf[1000];

static void read_write(int in, int out)
{
	for (int i = 0; i < 1000; i++) {
		buf[i].count = read(in, buf[i].buffer, BUFFLEN);
		if (buf[i].count <= 0)
			break;
	}

	for (int i = 0; i < 1000; i++) {
		if (buf[i].count <= 0)
			break;
		write(out, buf[i].buffer, buf[i].count);
	}
}

int main(int argc, char** argv)
{
	int fd0;
	int fd1;
	int nfds;
	int epollfd;

	#define MAX_EVENTS 10
	struct epoll_event ev, events[MAX_EVENTS];

	int flags = 0;
	if (argc > 1) {
		flags = IFF_NAPI | IFF_NAPI_FRAGS;
		printf("Use NAPI mode\n");
	}

	fd0 = open_tuntap("tun0", O_NONBLOCK, flags);
	if (!fd0)
		return 1;

	fd1 = open_tuntap("tun1", O_NONBLOCK, flags);
	if (!fd1)
		return 1;

	if (tuntap_system())
		return 1;

	epollfd = epoll_create1(0);
	if (epollfd == -1)
		return 1;

	ev.events = EPOLLIN;
	ev.data.fd = fd0;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd0, &ev) == -1)
		return 1;

	ev.events = EPOLLIN;
	ev.data.fd = fd1;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd1, &ev) == -1)
		return 1;

	if(daemon(0, 1))
		return 1;

	while (1) {
		nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (nfds == -1)
			return 1;
		for (int n = 0; n < nfds; ++n) {
			if (events[n].data.fd == fd0)
				read_write(fd0, fd1);
			else
				read_write(fd1, fd0);
		}
	}

	return 0;
}
