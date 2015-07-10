OUTPUT_ARCH(arm)

MEMORY
{
	RAMX (rx) : ORIGIN = 0x00100000, LENGTH = 0x00005000
	RAMRW (rw!i) : ORIGIN = 0x08000000, LENGTH = 0x00100000
}

SECTIONS
{
	.text : ALIGN(0x100) {
		build/crt0.o(.init)
		*(.text)
		*(.rodata)
	}

	.bss : {
		_bss_start = .;
		*(.bss);
	}
	_bss_end = .;
}

