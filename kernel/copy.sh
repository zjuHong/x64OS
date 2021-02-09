#!/bin/sh
make clean
make
sudo mount /dev/sdc1 /mnt
#sudo cp BootLoader.efi /mnt/EFI/BOOT/BOOTx64.EFI
sudo cp ./kernel.bin /mnt
sync
sudo umount /mnt
