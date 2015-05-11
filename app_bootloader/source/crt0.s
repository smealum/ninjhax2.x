.section ".init"
.arm
.align 4
.global _init
.global _start

_start:
	blx _main

_init:
	bx lr
