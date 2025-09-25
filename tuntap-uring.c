#include "tuntap.h"
#include <liburing.h>

#define NUMBER_BUFF 1000
#define QUEUE_DEPTH ((NUMBER_BUFF) * 2)

enum cmd {
	CMD_READ  = 0xA5,
	CMD_WRITE = 0x5A,
};

struct req {
	int      fd;
	size_t   size;
	enum cmd cmd;
	char     buffer[BUFFLEN];
};

static int submit_read(struct io_uring *ring, struct req *req)
{
	struct io_uring_sqe *sqe;

	req->cmd = CMD_READ;
	sqe = io_uring_get_sqe(ring);
	if (!sqe)
		printf("FULL\n");
	io_uring_prep_read(sqe, req->fd, req->buffer, sizeof(req->buffer), 0);
	io_uring_sqe_set_data(sqe, req);
}

static int submit_write(struct io_uring *ring, struct req *req)
{
	struct io_uring_sqe *sqe;

	req->cmd = CMD_WRITE;
	sqe = io_uring_get_sqe(ring);
	if (!sqe)
		printf("FULL\n");
	io_uring_prep_write(sqe, req->fd, req->buffer, req->size, 0);
	io_uring_sqe_set_data(sqe, req);
}

static int
parse_uring(struct io_uring *ring,
	    int fd0,
	    int fd1)
{
	struct req          *req;
	struct io_uring_cqe *cqe;

	int i = 0;
	int head;
	io_uring_for_each_cqe(ring, head, cqe) {
		req = io_uring_cqe_get_data(cqe);
		switch (req->cmd) {
		case CMD_READ:
			if (cqe->res > 0) {
				req->fd = req->fd == fd0 ? fd1 : fd0;
				req->size = cqe->res;
				submit_write(ring, req);
			} else {
				submit_read(ring, req);
			}
			break;
		case CMD_WRITE:
			req->fd = req->fd == fd0 ? fd1 : fd0;
			submit_read(ring, req);
			break;
		default:
			printf("BAD CMD %02X\n", req->cmd);
			exit(1);
		}
		i++;
	}
	io_uring_cq_advance(ring, i);
	io_uring_submit(ring);
}

int main(int argc, char** argv)
{
	struct io_uring       ring;
	int                   fd0;
	int                   fd1;
	struct req           *req0;
	struct req           *req1;
	
	int flags = 0;
	if (argc > 1) {
		//flags = IFF_NAPI | IFF_NAPI_FRAGS;
		flags = IFF_NAPI;
		printf("Use NAPI mode\n");
	}
	
	int ff = 0;
	ff = O_NONBLOCK;
	fd0 = open_tuntap("tun0", ff, flags);
	if (!fd0)
		return 1;

	fd1 = open_tuntap("tun1", ff, flags);
	if (!fd1)
		return 1;

	if (tuntap_system())
		return 1;

	req0 = malloc(sizeof(*req0) * NUMBER_BUFF);
	if (!req0)
		return 1;

	req1 = malloc(sizeof(*req1) * NUMBER_BUFF);
	if (!req1)
		return 1;

	if (io_uring_queue_init(NUMBER_BUFF * 4, &ring, 0))
		return 1;

	for (int i = 0; i < NUMBER_BUFF; i++) {
		req0[i].fd = fd0;
		req1[i].fd = fd1;
		submit_read(&ring, &req0[i]);
		submit_read(&ring, &req1[i]);
	}
	io_uring_submit(&ring);

	if(daemon(0, 1))
		return 1;

	while (1) {
		parse_uring(&ring, fd0, fd1);
	}
	return 0;
}
