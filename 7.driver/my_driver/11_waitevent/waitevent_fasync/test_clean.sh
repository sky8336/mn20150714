#!/bin/bash

MODULE=waitevent_fasync

echo -e "\n====== sudo rmmod $MODULE ======"
sudo rmmod $MODULE
dmesg
echo -e "\nls /dev/xhello-*"
ls /dev/xhello-*

echo -e "\n====== make clean ======"
make clean
rm a.out
