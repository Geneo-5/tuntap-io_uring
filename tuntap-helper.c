#include "tuntap.h"
#include <signal.h>

static const char const* system_cmds[] = {
	"ip link set tun0 netns ns0",
	"ip -n ns0 addr add 203.0.113.1/24 broadcast 203.0.113.255 dev tun0",
	"ip -n ns0 link set dev tun0 up",

	"ip link set tun1 netns ns1",
	"ip -n ns1 addr add 203.0.113.2/24 broadcast 203.0.113.255 dev tun1",
	"ip -n ns1 link set dev tun1 up",
};

static const char const* veth_system_cmds[] = {
	"ip link add veth0 type veth peer name tun0 netns ns0",
	"ip link set dev veth0 up",
	"ip -n ns0 addr add 203.0.113.1/24 broadcast 203.0.113.255 dev tun0",
	"ip netns exec ns0 ethtool --offload tun0 tx off",
	"ip -n ns0 link set dev tun0 up",

	"ip link add veth1 type veth peer name tun1 netns ns1",
	"ip link set dev veth1 up",
	"ip -n ns1 addr add 203.0.113.2/24 broadcast 203.0.113.255 dev tun1",
	"ip netns exec ns1 ethtool --offload tun1 tx off",
	"ip -n ns1 link set dev tun1 up",
};

static const char const* veth_exit_cmds[] = {
	"ip link del dev veth0",
	"ip link del dev veth1",
};

static const char HEX[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f',
};

static void hex(char* source, char* dest, ssize_t count)
{
	for (ssize_t i = 0; i < count; ++i) {
		unsigned char data = source[i];
		dest[2 * i] = HEX[data >> 4];
		dest[2 * i + 1] = HEX[data & 15];
	}
	dest[2 * count] = '\0';
}

void show_buffer(char *buff, size_t count)
{
	char buffer2[2*BUFFLEN + 1];
	hex(buff, buffer2, count);
	fprintf(stderr, "%s\n", buffer2);
}

void
print_debit(const char      *prefix,
            struct timespec  tstart,
            struct timespec  tend,
            size_t           nb)
{
	double sec;
	char *unit = "Mio";
	unsigned long deb = nb;

	sec = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - 
	      ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
	sec *= 1000 * 1000;
	deb *= 1000 * 1000;
	deb /= (unsigned long)sec;
	deb /= 1024*1024;
	printf("%s debit %ld%s (%ld)\n",prefix, deb, unit, nb);
}

int open_tuntap(char *name, int flags, int ifr_extra)
{
	int fd = open("/dev/net/tun", O_RDWR | flags);
	if (fd == -1)
		return 0;

	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI | ifr_extra;
	strncpy(ifr.ifr_name, name, IFNAMSIZ);
	if (ioctl(fd, TUNSETIFF, &ifr) < 0)
		return 0;

	return fd;
}

int system_call(const char const **cmds, size_t cnt, int force)
{
	while(cnt) {
		printf("call: %s\n", *cmds);
		if (system(*cmds) && !force)
			return 1;
		cmds++;
		cnt--;
	}
	return 0;
}

int tuntap_system(void)
{
	return system_call(system_cmds, array_nr(system_cmds), 0);
}

static void veth_term(int signum)
{
	system_call(veth_exit_cmds, array_nr(veth_exit_cmds), 1);
}

int veth_system(void)
{
	struct sigaction action;

	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = veth_term;
	sigaction(SIGTERM, &action, NULL);
	return system_call(veth_system_cmds, array_nr(veth_system_cmds), 0);
}

