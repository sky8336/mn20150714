#!/bin/bash

MODULE=async

echo "====== compile driver and test app ======"
make
gcc test.c

sudo dmesg  -C
echo -e "\n====== ls /dev/xhello-* ======"
ls /dev/xhello-*

echo -e "\n====== sudo insmod $MODULE.ko ======"
sudo insmod $MODULE.ko
echo "ls /dev/xhello-*"
ls /dev/xhello-*

echo -e "\n====== ./a.out /dev/xhello-0 ======"
sudo chmod 777 /dev/xhello-0
./a.out /dev/xhello-0

