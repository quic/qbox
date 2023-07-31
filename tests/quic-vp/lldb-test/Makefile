cdsp0_image: cdsp0_image.o
	hexagon-link -o $@ $<

cdsp0_image.o: cdsp0_image.S
	hexagon-llvm-mc -mv73 -filetype=obj -o $@ $<
