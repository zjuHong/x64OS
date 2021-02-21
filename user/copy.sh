#!/bin/sh
make clean
make
sudo mount /dev/sda1 /mnt
sudo cp ./init.bin /mnt
sync
sudo umount /mnt
