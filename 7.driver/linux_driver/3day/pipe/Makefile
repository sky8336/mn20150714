ifeq ($(KERNELRELEASE),)

#KERNELDIR ?= /home/lht/kernel2.6/linux-2.6.14

KERNELDIR ?= /lib/modules/$(shell uname -r)/build 
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module* module*

.PHONY: modules modules_install clean

else
    obj-m := pipe.o
endif

