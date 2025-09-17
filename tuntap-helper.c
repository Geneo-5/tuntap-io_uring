#include "tuntap.h"

static const char const* system_cmds[] = {
	"ip addr add 203.0.113.1/32 dev tun0",
	"ip link set dev tun0 txqueuelen 10000",
	"ip link set up dev tun0",
	"ip addr add 203.0.113.2/32 dev tun1",
	"ip link set dev tun1 txqueuelen 10000",
	"ip link set up dev tun1",
	"ip route add 203.0.113.2/32 dev tun0",
	"sysctl -w net.ipv4.conf.tun0.accept_local=1",
	"sysctl -w net.ipv4.conf.tun1.accept_local=1"
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

int open_tuntap(char *name, int flags, int ifr_extra)
{
	int fd = open("/dev/net/tun", O_RDWR | O_CLOEXEC | flags);
	if (fd == -1)
		return 0;
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI | ifr_extra;
	strncpy(ifr.ifr_name, name, IFNAMSIZ);
	if (ioctl(fd, TUNSETIFF, &ifr) == -1)
		return 0;
	return fd;
}

int system_call(const char const **cmds, size_t cnt)
{
	while(cnt) {
		printf("call: %s\n", *cmds);
		if (system(*cmds))
			return 1;
		cmds++;
		cnt--;
	}
	return 0;
}

int tuntap_system(void)
{
    return system_call(system_cmds, array_nr(system_cmds));
}

