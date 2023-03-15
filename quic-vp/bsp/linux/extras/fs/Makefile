filesystem.bin:
	dd if=/dev/zero of=$@ bs=4k count=256
	mkfs -t ext2 $@
