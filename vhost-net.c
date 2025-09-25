#include "tuntap.h"
#include <time.h>
#include <sys/epoll.h>
#include <linux/vhost.h>
#include <linux/virtio_net.h>
#include <linux/virtio_ring.h>
#include <stddef.h>
#include <sys/eventfd.h>
#include <string.h>
#include <errno.h>

#define HDR_LEN sizeof(struct virtio_net_hdr_mrg_rxbuf)

/* Used by implementation of kmalloc() in tools/virtio/linux/kernel.h */
void *__kmalloc_fake, *__kfree_ignore_start, *__kfree_ignore_end;

#include "linux/dma-mapping.h"
void kmsan_handle_dma(struct page *page, size_t offset, size_t size,
			     enum dma_data_direction dir)
{
}

enum {
	VHOST_NET_VQ_RX = 0,
	VHOST_NET_VQ_TX = 1,
	VHOST_NET_VQ_MAX = 2,
};

struct vq_info {
	int kick;
	int call;
	int idx;
	void *ring;
	/* copy used for control */
	struct vring vring;
	struct virtqueue *vq;
};

struct vdev_info {
	struct virtio_device vdev;
	int control;
	struct vq_info vqs[VHOST_NET_VQ_MAX];
	struct vhost_memory *mem;
};

struct buff {
	int used;
	struct scatterlist sl;
	char buffer[BUFFLEN];
};

#define DESC_NUM 1024
struct buff buf[DESC_NUM * 4] = { 0 };

static struct buff *
get_unused_buf()
{
	for (size_t i = 0; i < array_nr(buf); i++) {
		if (!buf[i].used) {
			buf[i].used = 1;
			return &buf[i];
		}
	}
	return NULL;
}

static void
free_buf(struct buff *b)
{
	if (b)
		b->used = 0;
}

static bool vq_notify(struct virtqueue *vq)
{
	struct vq_info *info = vq->priv;
	unsigned long long v = 1;
	int r;

	//printf("Notify %p\n", vq);

	r = write(info->kick, &v, sizeof(v));
	return r == sizeof(v);
}

static void vq_callback(struct virtqueue *vq)
{
}


static void fill_rx(struct vq_info *vq)
{
	struct buff *b;
	int ret;

	do {
		b = get_unused_buf();
		if (!b)
			break;

		sg_init_one(&b->sl, b->buffer, sizeof(b->buffer));
	} while(!virtqueue_add_inbuf(vq->vq, &b->sl, 1, b, GFP_ATOMIC));
	free_buf(b);
	virtqueue_kick(vq->vq);
}

static void flush_tx(struct vq_info *vq)
{
	struct buff *b;
	unsigned int len;

	while (b = virtqueue_get_buf(vq->vq, &len))
		free_buf(b);
}

static void
read_write(struct vdev_info *in,struct vdev_info *out)
{
	unsigned int len;
	unsigned long long val;
	struct buff *b;
	int r;

	read(in->vqs[VHOST_NET_VQ_RX].call, &val, sizeof(val));

	flush_tx(&out->vqs[VHOST_NET_VQ_TX]);
	while (b = virtqueue_get_buf(in->vqs[VHOST_NET_VQ_RX].vq, &len)) {
		sg_init_one(&b->sl, b->buffer, len);
		if (virtqueue_add_outbuf(out->vqs[VHOST_NET_VQ_TX].vq,
			&b->sl, 1, b, GFP_ATOMIC) < 0)
			free_buf(b);
	}
	virtqueue_kick(out->vqs[VHOST_NET_VQ_TX].vq);
	fill_rx(&in->vqs[VHOST_NET_VQ_RX]);
}

static int
vdev_info_init(struct vdev_info *dev, unsigned long long features)
{
	dev->vdev.features = features;
	INIT_LIST_HEAD(&dev->vdev.vqs);
	spin_lock_init(&dev->vdev.vqs_list_lock);

	dev->control = open("/dev/vhost-net", O_RDWR);
	if (dev->control < 0)
		return 1;

	if (ioctl(dev->control, VHOST_SET_OWNER, NULL) < 0)
		return 1;

	dev->mem = malloc(offsetof(struct vhost_memory, regions) +
			  sizeof(dev->mem->regions[0]));
	if (!dev->mem)
		return 1;

	memset(dev->mem, 0, offsetof(struct vhost_memory, regions) +
	       sizeof(dev->mem->regions[0]));
	dev->mem->nregions = 1;
	dev->mem->regions[0].guest_phys_addr = (long)buf;
	dev->mem->regions[0].userspace_addr = (long)buf;
	dev->mem->regions[0].memory_size = sizeof(buf);

	if (ioctl(dev->control, VHOST_SET_MEM_TABLE, dev->mem) < 0)
		return 1;

	if (ioctl(dev->control, VHOST_SET_FEATURES, &features) < 0)
		return 1;

	return 0;
}

static int
vq_reset(struct vq_info *info, int num, struct virtio_device *vdev)
{
	if (info->vq)
		vring_del_virtqueue(info->vq);

	memset(info->ring, 0, vring_size(num, 4096));
	vring_init(&info->vring, num, info->ring, 4096);
	info->vq = vring_new_virtqueue(info->idx, num, 4096, vdev, true, false,
				       info->ring, vq_notify, vq_callback, "test");
	if (!info->vq)
		return 1;

	info->vq->priv = info;
	return 0;
}

static int
vhost_vq_setup(struct vdev_info *dev, struct vq_info *info)
{
	struct vhost_vring_addr addr = {
		.index = info->idx,
		.desc_user_addr = (uint64_t)(unsigned long)info->vring.desc,
		.avail_user_addr = (uint64_t)(unsigned long)info->vring.avail,
		.used_user_addr = (uint64_t)(unsigned long)info->vring.used,
	};
	struct vhost_vring_state state = { .index = info->idx };
	struct vhost_vring_file file = { .index = info->idx };
	int ret;

	state.num = info->vring.num; // set max number descriptor
	if (ioctl(dev->control, VHOST_SET_VRING_NUM, &state) < 0) {
		printf("VHOST_SET_VRING_NUM fail: %s\n", strerror(errno));
		return 1;
	}

	state.num = 0; // set descriptor base
	if (ioctl(dev->control, VHOST_SET_VRING_BASE, &state) < 0) {
		printf("VHOST_SET_VRING_BASE fail: %s\n", strerror(errno));
		return 1;
	}

	if (ioctl(dev->control, VHOST_SET_VRING_ADDR, &addr) < 0) {
		printf("VHOST_SET_VRING_ADDR fail: %s\n", strerror(errno));
		return 1;
	}

	file.fd = info->kick;
	if (ioctl(dev->control, VHOST_SET_VRING_KICK, &file) < 0) {
		printf("VHOST_SET_VRING_KICK fail: %s\n", strerror(errno));
		return 1;
	}

	file.fd = info->call;
	if (ioctl(dev->control, VHOST_SET_VRING_CALL, &file) < 0) {
		printf("VHOST_SET_VRING_CALL fail: %s\n", strerror(errno));
		return 1;
	}

	return 0;
}
static int
vq_info_add(struct vdev_info *dev, int idx, int num, int fd)
{
	struct vhost_vring_file backend = { .index = idx, .fd = fd };
	struct vq_info *info = &dev->vqs[idx];

	info->idx = idx;
	info->kick = eventfd(0, EFD_NONBLOCK);
	info->call = eventfd(0, EFD_NONBLOCK);
	if (posix_memalign(&info->ring, 4096, vring_size(num, 4096)) < 0) {
		printf("Fail to alloc %d\n", vring_size(num, 4096));
		return 1;
	}

	if (vq_reset(info, num, &dev->vdev))
		return 1;

	if (vhost_vq_setup(dev, info))
		return 1;

	if (ioctl(dev->control, VHOST_NET_SET_BACKEND, &backend)) {
		printf("Set backend fail: %s\n", strerror(errno));
		return 1;
	}

	return 0;
}

int main(int argc, char** argv)
{
	int fd0;
	int fd1;
	int nfds;
	int epollfd;
	int len = HDR_LEN;
	unsigned long long features = 0;

	features |= (1ULL << VIRTIO_RING_F_INDIRECT_DESC);
	features |= (1ULL << VIRTIO_RING_F_EVENT_IDX);
	features |= (1ULL << VIRTIO_F_VERSION_1);

	#define MAX_EVENTS 10
	struct epoll_event ev, events[MAX_EVENTS];

	int flags = IFF_VNET_HDR;
	if (argc > 1) {
		flags |= IFF_NAPI | IFF_NAPI_FRAGS;
		printf("Use NAPI mode\n");
	}

	fd0 = open_tuntap("tun0", 0, flags);
	if (!fd0)
		return 1;
	
	fd1 = open_tuntap("tun1", 0, flags);
	if (!fd1)
		return 1;

	if (tuntap_system())
		return 1;

	if(daemon(0, 1))
		return 1;

	struct vdev_info dev0 = { 0 };
	if (vdev_info_init(&dev0, features))
		return 1;

	if (ioctl(fd0, TUNSETVNETHDRSZ, &len))
		return 1;

	if (vq_info_add(&dev0, VHOST_NET_VQ_RX, DESC_NUM, fd0))
		return 1;

	if (vq_info_add(&dev0, VHOST_NET_VQ_TX, DESC_NUM, fd0))
		return 1;

	struct vdev_info dev1 = { 0 };
	if (vdev_info_init(&dev1, features))
		return 1;


	if (ioctl(fd1, TUNSETVNETHDRSZ, &len))
		return 1;

	if (vq_info_add(&dev1, VHOST_NET_VQ_RX, DESC_NUM, fd1))
		return 1;

	if (vq_info_add(&dev1, VHOST_NET_VQ_TX, DESC_NUM, fd1))
		return 1;

	epollfd = epoll_create1(0);
	if (epollfd == -1)
		return 1;

	ev.events = EPOLLIN;
	ev.data.ptr = &dev0;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, dev0.vqs[VHOST_NET_VQ_RX].call, &ev) == -1)
		return 1;

	ev.events = EPOLLIN;
	ev.data.ptr = &dev1;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, dev1.vqs[VHOST_NET_VQ_RX].call, &ev) == -1)
		return 1;

	fill_rx(&dev0.vqs[VHOST_NET_VQ_RX]);
	fill_rx(&dev1.vqs[VHOST_NET_VQ_RX]);


	while (1) {
		nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		for (int n = 0; n < nfds; ++n) {
			if (events[n].data.ptr == &dev0)
				read_write(&dev0, &dev1);
			else
				read_write(&dev1, &dev0);
		}
	}

	return 0;
}
