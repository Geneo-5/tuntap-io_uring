
CFLAGS += $(shell pkg-config --libs --cflags liburing) -lc

SUDO     := sudo
CURDIR   := $(PWD)
BUILDDIR := $(CURDIR)/build
bin      := tuntap tuntap-uring veth vhost-net

LINUX_SRC := /home/lojs/Projects/icsw/src/linux

CFLAGS4LINUX := -I$(LINUX_SRC)/tools/virtio/
CFLAGS4LINUX += -I$(LINUX_SRC)/tools/include/
CFLAGS4LINUX += -I$(LINUX_SRC)/usr/include/
CFLAGS4LINUX += -include $(LINUX_SRC)/include/linux/kconfig.h
CFLAGS4LINUX += -pthread
CFLAGS4LINUX += -include fixup.h 
#-include $(LINUX_SRC)/tools/virtio/linux/kmsan.h

virtio_ring := $(LINUX_SRC)/drivers/virtio/virtio_ring.c


.PHONY: all
all: $(addprefix $(BUILDDIR)/,$(bin))
	@$(SUDO) $(CURDIR)/test.sh "$(BUILDDIR)"

$(BUILDDIR)/vhost-net: vhost-net.c $(virtio_ring) tuntap-helper.c | $(BUILDDIR)
	@echo Build $(@)
	gcc -o $(@) $(CFLAGS) $(CFLAGS4LINUX) $(^) 

$(BUILDDIR)/%: %.c tuntap-helper.c | $(BUILDDIR)
	@echo Build $(@)
	@gcc -o $(@) $(^) $(CFLAGS)

.PHONY: build
build: $(addprefix $(BUILDDIR)/,$(bin))

.PHONY: clean
clean:
	@rm -rf $(BUILDDIR)

$(BUILDDIR):
	@mkdir -p $(@)

