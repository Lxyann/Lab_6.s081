	# qemu -kernel loads the kernel at 0x80000000
        # and causes each CPU to jump there.
        # kernel.ld causes the following code to
        # be placed at 0x80000000.
.section .text
_entry:
	# set up a stack for C.
        # stack0 is declared in start.c,
        # with a 4096-byte stack per CPU.
        # sp = stack0 + (hartid * 4096)
        la sp, stack0   #0x80009830
        li a0, 1024*4   #4096
	csrr a1, mhartid #0
        addi a1, a1, 1   #1
        mul a0, a0, a1   #4096
        add sp, sp, a0   #0x8000a830
	# jump to start() in start.c
        call start
spin:
        j spin
