ifeq ($(KERNELRELEASE),)

#KERNELDIR ?= /home/lht/kernel2.6/linux-2.6.14

KERNELDIR ?= /home/linux/FS_S5PC100/linux-2.6.35-farsight/
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module* module* a.out

.PHONY: modules modules_install clean

else
    obj-m := s5pc100_led.o
endif

