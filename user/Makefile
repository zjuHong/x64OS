CFLAGS := -mcmodel=large -fno-builtin -m64

all: system_api_lib
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system_api_lib init.bin

system_api_lib: init.o lib.o errno.o malloc.o
	ld -b elf64-x86-64 -z muldefs -o system_api_lib init.o lib.o errno.o malloc.o -T User.lds

init.o: init.c
	gcc-4.8  $(CFLAGS) -c init.c

lib.o: lib.c
	gcc-4.8  $(CFLAGS) -c lib.c

errno.o: errno.c
	gcc-4.8  $(CFLAGS) -c errno.c

malloc.o: malloc.c
	gcc-4.8  $(CFLAGS) -c malloc.c

clean:
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ Makefile~ User.lds~ system_api_lib init.bin

