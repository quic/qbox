
PREFIX=
all:
	$(PREFIX)arm-none-eabi-as ./loader-test.asm -o loader-test.o -mthumb
	$(PREFIX)arm-none-eabi-as ./loader-test-bin.asm -o loader-test-bin.o -mthumb
	$(PREFIX)arm-none-eabi-ld ./loader-test.o ./loader-test-bin.o -o loader-test.elf -T ./link.ld
	$(PREFIX)arm-none-eabi-objcopy loader-test.elf -O binary loader-test.bin

clean:
	rm loader-test.o loader-test-bin.o loader-test.elf loader-test.bin
