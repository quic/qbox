.DEFAULT_GOAL := all

CC=hexagon-clang
OBJCOPY=hexagon-llvm-objcopy
O=BUILD

CFLAGS+=-G0 -O2
CRT_OBJS=boot.o
LDFLAGS+= -static

TESTS += \
	standalone_hw.elf \
	qtimer_test.elf \
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
qtimer_test.elf: qtimer_test.o

$(TESTS):
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $^ -o $@ $(LDFLAGS) 

clean:
	rm *.o *.bin *.elf

qtimer_test.o: CFLAGS+= -DCSR_BASE=0xfc900000 -DQTMR_FREQ=19200000 -DIRQ1=3 -DIRQ2=4

all: standalone_hw.bin qtimer_test.bin
