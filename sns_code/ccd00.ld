OUTPUT_ARCH(arm)

MEMORY
{
	RAMRW (rwx) : ORIGIN = 0x0011D300, LENGTH = 0x00000D00
}

SECTIONS
{
	.text : ALIGN(0x100) {
		build/crt0.o(.init)
		*(.text)
		*(.rodata)
	}

	data : ALIGN(0x1000) {
		*(data)
	}
}

