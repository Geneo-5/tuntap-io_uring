#include "tuntap.h"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

#define NBITER 1000

char buffer[NBITER][BUFFLEN];
struct iovec   buf[NBITER];
struct mmsghdr msg[NBITER];

static void read_write(int in, int out)
{
	for (int i = 0; i < NBITER; i++)
		buf[i].iov_len = BUFFLEN;

	int nb = recvmmsg(in, msg, NBITER, 0, NULL);
	for (int i = 0; i < nb; i++)
		buf[i].iov_len = msg[i].msg_len;

	sendmmsg(out, msg, nb, 0);
}

int main(int argc, char** argv)
{
	int fd0;
	int fd1;
	int nfds;
	int epollfd;
	struct sockaddr_ll sockaddr = {
		.sll_family = AF_PACKET,
		.sll_protocol = htons(ETH_P_ALL),
	};

	#define MAX_EVENTS 10
	struct epoll_event ev, events[MAX_EVENTS];

	if (veth_system())
		return 1;

	fd0 = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (!fd0)
		return 1;
	sockaddr.sll_ifindex = if_nametoindex("veth0");
	bind(fd0, (const struct sockaddr *)&sockaddr, sizeof(sockaddr));
	fcntl(fd0, F_SETFL, fcntl(fd0, F_GETFL, 0) | O_NONBLOCK);

	fd1 = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (!fd1)
		return 1;

	sockaddr.sll_ifindex = if_nametoindex("veth1");
	bind(fd1, (const struct sockaddr *)&sockaddr, sizeof(sockaddr));
	fcntl(fd1, F_SETFL, fcntl(fd1, F_GETFL, 0) | O_NONBLOCK);

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
	
	memset(msg, 0, sizeof(msg));
	for (int i = 0; i < NBITER; i++) {
		buf[i].iov_base = buffer[i];
		msg[i].msg_hdr.msg_iov = &buf[i];
		msg[i].msg_hdr.msg_iovlen = 1;
	}

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
