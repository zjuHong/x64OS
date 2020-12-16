#!/bin/sh
cd kernel
make clean
make
cd ..
dd if=./bootloader/boot.bin of=./boot.img bs=512 count=1 conv=notrunc
sudo mount boot.img /media/ -t vfat -o loop
sudo cp ./bootloader/loader.bin /media/
sudo cp ./kernel/kernel.bin /media/
sync
sudo umount /media/
sudo bochs -qf bochsrc

