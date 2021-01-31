#!/bin/sh
sudo mount /dev/sdb1 /mnt
sudo cp BootLoader.efi /mnt/EFI/BOOT/BOOTx64.EFI
sudo cp ../kernel/kernel.bin /mnt
sync
sudo umount /mnt
