obj-m := kds.o

CONFIG_MODULE_SIG=n
# KDIR := /path/to/kernel/sources/root/directory
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: kds.c
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
