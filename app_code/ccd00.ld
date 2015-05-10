OUTPUT_ARCH(arm)

MEMORY
{
	RAMX (rx) : ORIGIN = 0x00200000, LENGTH = 0x00010000
	RAMRW (rw!i) : ORIGIN = 0x0046F000, LENGTH = 0x00099000
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

