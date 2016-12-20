ifeq ($(KERNELRELEASE),)

#KERNELDIR ?= /your/target/source/directory/
KERNELDIR ?= /media/MQ/farsight/S5PC100/kernel/linux-2.6.35
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module* modules*

.PHONY: modules modules_install clean

else
    obj-m := s5pc100_wdt.o
endif
