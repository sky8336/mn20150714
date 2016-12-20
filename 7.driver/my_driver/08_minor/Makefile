KERNELDIR ?=/lib/modules/$(shell uname -r)/build

module:
	$(MAKE) -C $(KERNELDIR) M=$(shell pwd) modules
clean:
	rm -rf *.ko *.o *.mod* modules* Mo*

obj-m=minor.o
