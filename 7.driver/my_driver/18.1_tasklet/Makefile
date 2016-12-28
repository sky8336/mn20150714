#KERNELDER ?= /lib/modules/$(shell uname -r)/build
KERNELDER ?= /home/linux/linux-2.6.35

module:
	$(MAKE) -C $(KERNELDER) M=$(shell pwd) modules

clean:
	rm -rf *.ko *.o *.mod* modules* Mo* .*.cmd .tmp_versions

obj-m=tasklet.o
