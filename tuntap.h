#ifndef _TUNTAP_H_
#define _TUNTAP_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include <net/if.h> // ifreq
#include <linux/if_tun.h> // IFF_TUN, IFF_NO_PI
#include <linux/if_arp.h>

#include <sys/ioctl.h>

#define _compile_eval(_expr, _msg) \
	(!!sizeof(struct { static_assert(_expr, _msg); int _dummy; }))

#define compile_eval(_expr, _stmt, _msg) \
	(_compile_eval(_expr, "compile_eval(" # _expr "): " _msg) ? (_stmt) : \
	                                                            (_stmt))

#define _is_same_type(_a, _b) \
	__builtin_types_compatible_p(typeof(_a), typeof(_b))

#define _is_array(_array) \
	(!_is_same_type(_array, &(_array)[0]))

#define array_nr(_array) \
	compile_eval(_is_array(_array), \
	             sizeof(_array) / sizeof((_array)[0]), \
	             "array expected")

#define BUFFLEN (4 * 1024)

extern void show_buffer(char *buff, size_t count);
extern int open_tuntap(char *name, int flags);
extern int system_call(const char const **cmds, size_t cnt);
extern int tuntap_system(void);

#endif /* _TUNTAP_H_ */
