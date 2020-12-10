dd if=./bootloader/boot.bin of=./boot.img bs=512 count=100 conv=notrunc
sudo mount boot.img /media/ -t vfat -o loop
sudo cp ./bootloader/loader.bin /media/
sudo cp ./kernel/kernel.bin /media/
sync
sudo umount /media/
bochs -f bochsconfig

