CFLAGS := -mcmodel=large -fno-builtin -fno-stack-protector -m64

ASFLAGS := --64

PIC := APIC

all: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin

system:	head.o entry.o main.o printk.o trap.o memory.o interrupt.o task.o keyboard.o
	ld -b elf64-x86-64 -z muldefs -o system head.o entry.o main.o printk.o trap.o memory.o interrupt.o task.o keyboard.o -T Kernel.lds 

head.o:	head.S
	gcc -E head.S > head.s
	as $(ASFLAGS) -o head.o head.s

entry.o: entry.S
	gcc -E entry.S > entry.s
	as $(ASFLAGS) -o entry.o entry.s
	
main.o:	main.c
	gcc  $(CFLAGS) -c main.c
	
printk.o: printk.c
	gcc  $(CFLAGS) -c printk.c

trap.o: trap.c
	gcc  $(CFLAGS) -c trap.c

memory.o: memory.c
	gcc  $(CFLAGS) -c memory.c

interrupt.o: interrupt.c
	gcc  $(CFLAGS) -c interrupt.c

task.o: task.c
	gcc  $(CFLAGS) -c task.c

keyboard.o: keyboard.c
	gcc  $(CFLAGS) -c keyboard.c

clean:
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ system  Makefile~ Kernel.lds~ kernel.bin 

