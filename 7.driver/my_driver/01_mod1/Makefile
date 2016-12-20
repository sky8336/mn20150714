KERNELDIR ?=/lib/modules/$(shell uname -r)/build

module:
	$(MAKE) -C $(KERNELDIR) M=$(shell pwd) modules
clean:
	rm -rf *.ko hello*.o hello*.mod* modules* Mo*

obj-m=hello.o hello_v1.o
