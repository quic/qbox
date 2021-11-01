.DEFAULT_GOAL := all

CC=hexagon-clang
OBJCOPY=hexagon-llvm-objcopy
O=BUILD

CFLAGS+=-G0 -O2
CRT_OBJS=boot.o
LDFLAGS+= -static

TESTS += \
	standalone_hw.elf \
	$()

%.o: %.S
	$(CC) $(ASFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
%.o: %.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
%.o: %.cpp
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

standalone_%: LDFLAGS+= -lc -lstandalone -mhvx -fvectorize
standalone_hw.elf: standalone_hw.o monitor_insts.o

$(TESTS):
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $^ -o $@ $(LDFLAGS) 

clean:
	rm *.o *.bin *.elf

all: standalone_hw.bin
