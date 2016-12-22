ifeq ($(KERNELRELEASE),)

KERNELDIR ?= /media/MQ/farsight/S5PC100/kernel/linux-2.6.35/
CC = /media/MQ/toolchain/arm-2010.09/bin/arm-none-linux-gnueabi-gcc

PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

test:test.c


clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module* module* test

.PHONY: modules modules_install clean

else
    obj-m := s5pc100_temp.o
endif

