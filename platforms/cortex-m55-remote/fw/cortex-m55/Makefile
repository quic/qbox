NAME=$(shell basename $(CURDIR))

CC=arm-none-eabi-gcc

all:
	$(CC) -nostdinc -nostdlib -mcpu=cortex-m55 -O2 -mthumb -I. -c setup.s -o setup.o
	$(CC) -nostdinc -nostdlib -mcpu=cortex-m55 -O2 -mthumb -I. -c nvic.c -o nvic.o
	$(CC) -nostdinc -nostdlib -mcpu=cortex-m55 -O2 -mthumb -I. -c main.c -o main.o
	$(CC) -nostdinc -nostdlib -mcpu=cortex-m55 -O2 -mthumb -I. setup.o nvic.o main.o -o $(NAME).elf -Wl,-Tlinker.ld
	arm-none-eabi-objcopy -Obinary $(NAME).elf $(NAME).bin
	xxd -i $(NAME).bin

clean:
	rm -f *.o *.elf *.bin

