ifeq ($(KERNELRELEASE),)

#KERNELDIR ?= /your/target/source/directory/
KERNELDIR ?= /home/farsight/test_kernel/android-2.6.29-samsung 
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module* modules*

.PHONY: modules modules_install clean

else
    obj-m := m25p10a.o
endif
