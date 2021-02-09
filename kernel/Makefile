CFLAGS := -mcmodel=large -fno-builtin -fno-stack-protector -m64

ASFLAGS := --64

PIC := APIC

all: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin


system:	head.o entry.o main.o printk.o trap.o memory.o interrupt.o PIC.o task.o cpu.o keyboard.o mouse.o disk.o SMP.o time.o HPET.o softirq.o timer.o schedule.o fat32.o
	ld -b elf64-x86-64 -z muldefs -o system head.o entry.o main.o printk.o trap.o memory.o interrupt.o PIC.o task.o cpu.o keyboard.o mouse.o disk.o SMP.o time.o HPET.o softirq.o timer.o schedule.o fat32.o -T Kernel.lds 


head.o:	head.S
	gcc-4.8 -E head.S > head.s
	as $(ASFLAGS) -o head.o head.s

entry.o: entry.S
	gcc-4.8 -E entry.S > entry.s
	as $(ASFLAGS) -o entry.o entry.s

main.o:	main.c
	gcc-4.8  $(CFLAGS) -c main.c -D$(PIC)

printk.o: printk.c
	gcc-4.8  $(CFLAGS) -c printk.c

trap.o: trap.c
	gcc-4.8  $(CFLAGS) -c trap.c

memory.o: memory.c
	gcc-4.8  $(CFLAGS) -c memory.c

interrupt.o: interrupt.c
	gcc-4.8  $(CFLAGS) -c interrupt.c

ifeq ($(PIC),APIC)
PIC.o: APIC.c
	gcc-4.8  $(CFLAGS) -c APIC.c -o PIC.o
else
PIC.o: 8259A.c
	gcc-4.8  $(CFLAGS) -c 8259A.c -o PIC.o
endif
	
task.o: task.c
	gcc-4.8  $(CFLAGS) -c task.c	

cpu.o: cpu.c
	gcc-4.8  $(CFLAGS) -c cpu.c

keyboard.o: keyboard.c
	gcc-4.8  $(CFLAGS) -c keyboard.c

mouse.o: mouse.c
	gcc-4.8  $(CFLAGS) -c mouse.c

disk.o: disk.c
	gcc-4.8  $(CFLAGS) -c disk.c

SMP.o: SMP.c
	gcc-4.8  $(CFLAGS) -c SMP.c

time.o: time.c
	gcc-4.8  $(CFLAGS) -c time.c

HPET.o: HPET.c
	gcc-4.8  $(CFLAGS) -c HPET.c

softirq.o: softirq.c
	gcc-4.8  $(CFLAGS) -c softirq.c

timer.o: timer.c
	gcc-4.8  $(CFLAGS) -c timer.c

schedule.o: schedule.c
	gcc-4.8  $(CFLAGS) -c schedule.c

fat32.o: fat32.c
	gcc  $(CFLAGS) -c fat32.c

clean:
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ system  Makefile~ Kernel.lds~ kernel.bin 

