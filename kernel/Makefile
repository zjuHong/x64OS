all: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin

system:	head.o entry.o main.o printk.o trap.o memory.o interrupt.o task.o
	ld -b elf64-x86-64 -z muldefs -o system head.o entry.o main.o printk.o trap.o memory.o interrupt.o task.o -T Kernel.lds 

head.o:	head.S
	gcc-4.8 -E  head.S > head.s
	as --64 -o head.o head.s

entry.o: entry.S
	gcc-4.8 -E  entry.S > entry.s
	as --64 -o entry.o entry.s
	
main.o:	main.c
	gcc-4.8  -mcmodel=large -fno-builtin -m64 -c main.c
	
printk.o: printk.c
	gcc-4.8  -mcmodel=large -fno-builtin -fno-stack-protector -m64 -c printk.c

trap.o: trap.c
	gcc-4.8  -mcmodel=large -fno-builtin -m64 -c trap.c

memory.o: memory.c
	gcc-4.8  -mcmodel=large -fno-builtin -m64 -c memory.c

interrupt.o: interrupt.c
	gcc-4.8  -mcmodel=large -fno-builtin -m64 -c interrupt.c
	
task.o: task.c
	gcc-4.8  -mcmodel=large -fno-builtin -fno-stack-protector -m64 -c task.c	

clean:
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ system  Makefile~ Kernel.lds~ kernel.bin 

