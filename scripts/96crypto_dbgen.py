# this takes a single argument : the path to the decrypted and decompressed code.bin file for a game
# it outputs the xml file to stdout
# you should place the XML output in a file on your sdcard at the following path : sdmc:/mmap/<TID>.xml
# example use :
# 	python 96crypto_dbgen.py triforce_heroes_demo_code.bin > 0004000000182200.xml
# 	cp 0004000000182200.xml E:/mmap/0004000000182200.xml

import sys
import struct

code_fn = sys.argv[1]

def getProcessMap(code):
	size = len(code)
	header = {}
	map = []

	# init
	header["text_end"] = 0x00160000
	header["data_address"] = 0x00100000 + (size & ~0xfff) - 0x8000
	header["data_size"] = 0x0
	header["processAppCodeAddress"] = 0x00105000
	header["num"] = 0

	# find hook target
	# assume second instruction is a jump. if it's not we're fucked anyway so it's ok to ignore that possibility i guess.
	header["processHookAddress"] = (((struct.unpack("<I", code[4:8])[0] & 0x00ffffff) << 2) + 0x0010000C) & ~0x1f
	if header["processHookAddress"] < 0x00105000 or header["processHookAddress"] > 0x0010A000:
		header["processAppCodeAddress"] = 0x00105000
	else:
		header["processAppCodeAddress"] = 0x0010B000

	# figure out the physical memory map
	current_size = size
	current_offset = 0x00000000
	for i in range(3):
		mask = 0x000fffff >> (4 * i)
		section_size = current_size & (~mask)
		if section_size != 0:
			if header["num"] == 0:
				header["processLinearOffset"] = section_size
			current_offset += section_size;
			map += [[(0x00100000 + current_offset - section_size - 0x00008000), - (current_offset - header["processLinearOffset"]), section_size]]
			header["num"] += 1
			current_size -= section_size

	map[0][0] += 0x00008000
	map[0][1] += 0x00008000
	map[0][2] -= 0x00008000

	return (header, map)

code_data = open(code_fn, "rb").read()
header, map = getProcessMap(code_data)

print("<header>")
for k in header:
	print("	<%s>0x%X</%s>" % (k, header[k], k))
print("</header>")

print("<map>")
for m in map:
	print("	<entry>")
	print("		<src>%s</src>" % hex(m[0]))
	print("		<dst>%s</dst>" % hex(m[1]))
	print("		<size>%s</size>" % hex(m[2]))
	print("	</entry>")
print("</map>")
