#!/bin/bash

MODULE=atomic

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

echo -e "\n====== ./a.out ======"
sudo ./a.out

echo -e "\n====== sudo rmmod $MODULE ======"
sudo rmmod $MODULE
dmesg
echo -e "\nls /dev/xhello-*"
ls /dev/xhello-*

echo -e "\n====== make clean ======"
make clean
rm a.out
