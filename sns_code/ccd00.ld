OUTPUT_ARCH(arm)

MEMORY
{
	RAMRW (rx) : ORIGIN = 0x0011D300, LENGTH = 0x00000D00
}

SECTIONS
{
	.text : ALIGN(0x100) {
		build/crt0.o(.init)
		*(.text)
		*(.rodata)
	}
}

