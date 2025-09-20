#define _GNU_SOURCE
#include "tuntap.h"
#include <time.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <signal.h>

#define NBITER 1000

char           buf[NBITER][BUFFLEN];
struct iovec   iovecs[NBITER];
struct mmsghdr msgs[NBITER] = { 0 };

static const char const* veth_system_cmds[] = {
	"ip netns add ns0",
	"ip link add veth0 type veth peer name tun0 netns ns0",
	"ip link set dev veth0 up",
	"ip -n ns0 addr add 203.0.113.1/24 dev tun0",
	"ip netns exec ns0 ethtool --offload tun0 tx off",
	"ip -n ns0 link set dev tun0 txqueuelen 10000",
	"ip -n ns0 link set dev tun0 up",
	"ip netns add ns1",
	"ip link add veth1 type veth peer name tun1 netns ns1",
	"ip link set dev veth1 up",
	"ip -n ns1 addr add 203.0.113.2/24 dev tun1",
	"ip netns exec ns1 ethtool --offload tun1 tx off",
	"ip -n ns1 link set dev tun1 txqueuelen 10000",
	"ip -n ns1 link set dev tun1 up",
};

static const char const* veth_exit_cmds[] = {
	"ip link del dev veth0 || true",
	"ip link del dev veth1 || true",
	"ip netns del ns0 || true",
	"ip netns del ns1 || true",
};

void term(int signum)
{
	system_call(veth_exit_cmds, array_nr(veth_exit_cmds));
}

static void read_write(int in, int out)
{
	int nb;
	
	for (int i = 0; i < NBITER; i++)
		iovecs[i].iov_len = BUFFLEN;

	nb = recvmmsg(in, msgs, NBITER, 0, NULL);
	if (nb <= 0)
		return;

	for (int i = 0; i < nb; i++)
		iovecs[i].iov_len = msgs[i].msg_len;

	sendmmsg(out, msgs, nb, 0);

}

int main(int argc, char** argv)
{
	int fd0;
	int fd1;
	int nfds;
	int epollfd;
	uint32_t bypass = 1;
	struct sockaddr_ll sockaddr = {
		.sll_family = AF_PACKET,
		.sll_protocol = htons(ETH_P_ALL)
	};

	#define MAX_EVENTS 10
	struct epoll_event ev, events[MAX_EVENTS];

	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGTERM, &action, NULL);

	if (system_call(veth_system_cmds, array_nr(veth_system_cmds)))
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

	for (int i = 0; i < NBITER; i++) {
		iovecs[i].iov_base         = buf[i];
		msgs[i].msg_hdr.msg_iov    = &iovecs[i];
		msgs[i].msg_hdr.msg_iovlen = 1;
	}

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
