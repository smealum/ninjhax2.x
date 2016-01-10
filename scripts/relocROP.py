import sys
from datetime import datetime
import ast

def outputConstantsH(d):
	out=""
	out+=("#ifndef CONSTANTS_H")+"\n"
	out+=("#define CONSTANTS_H")+"\n"
	for k in d:
		out+=("	#define "+k[0]+" "+str(k[1]))+"\n"
	out+=("#endif")+"\n"
	return out

def outputConstantsS(d):
	out=""
	for k in d:
		out+=(k[0]+" equ ("+str(k[1])+")")+"\n"
	return out

data_fn = sys.argv[1]

def getWord(b, k, n=4):
	return sum(list(map(lambda c: b[k+c]<<(c*8),range(n))))

_got_start = None
_got_end = None
_mini_got_start = None
_mini_got_end = None

for l in sys.stdin:
	l = l.split()
	adr = int("0x" + l[1], 0)
	name = l[-1]
	if name == "_got_start":
		_got_start = adr
	elif name == "_got_end":
		_got_end = adr
	if name == "_mini_got_start":
		_mini_got_start = adr
	elif name == "_mini_got_end":
		_mini_got_end = adr

base_adr = 0x00105000

data = bytearray(open(data_fn, "rb").read())

print(".macro relocate")
for i in range(_got_start - base_adr, _got_end - base_adr, 0x4):
	val = getWord(data, i)
	if val >= base_adr and val < 0x08000000:
		print("	add_and_store 0xBABE0007, "+hex(val - base_adr)+", MENU_OBJECT_LOC + appCode - object + "+hex(i))
for i in range(_mini_got_start - base_adr, _mini_got_end - base_adr, 0x4):
	val = getWord(data, i)
	if val >= base_adr and val < 0x08000000:
		print("	add_and_store 0xBABE0007, "+hex(val - base_adr)+", MENU_OBJECT_LOC + appCode - object + "+hex(i))
print(".endmacro")
