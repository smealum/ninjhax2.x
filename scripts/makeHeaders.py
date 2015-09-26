from datetime import datetime
import sys
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

def outputConstantsPY(d):
	out=""
	for k in d:
		out+=(k[0]+" = ("+str(k[1])+")")+"\n"
	return out

if len(sys.argv)<8:
	print("use : "+sys.argv[0]+" <firmver> <cnver> <msetver> <rover> <menuver> <region> <outname> <extensionless_output_name> <input_file1> <input_file2> ...")
	exit()

# l=[("_SPIDER_VERSION", sys.argv[3]),
l=[("_RO_VERSION", sys.argv[4])]
l+=[("FIRM_VERSION", "\""+sys.argv[1]+"\""),
	("CN_VERSION", "\""+sys.argv[2]+"\""),
	("MSET_VERSION", sys.argv[3]),
	("RO_VERSION", "\""+sys.argv[4]+"\""),
	("MENU_VERSION", "\""+sys.argv[5]+"\""),
	("REGION", "\""+sys.argv[6]+"\""),
	("REGION_ID", str({"J" : 0, "U" : 1, "E" : 2}[sys.argv[6]])),
	("IS_N3DS", str(1 if sys.argv[1][0:4] == "N3DS" else 0)),
	("CN_NINJHAX_URL", "\"http://192.168.109.1/\""),
	# ("CN_NINJHAX_URL", "\"http://smealum.github.io/ninjhax2/rMVPFSoVNs/p/\""),
	("OUTNAME", "\""+sys.argv[7]+"\"")]
l+=[("BUILDTIME", "\""+datetime.now().strftime("%Y-%m-%d %H:%M:%S")+"\"")]
l+=[("HB_NUM_HANDLES", "16")]

for fn in sys.argv[9:]:
	s=open(fn,"r").read()
	if len(s)>0:
		l+=(ast.literal_eval(s))

open(sys.argv[8]+".h","w").write(outputConstantsH(l))
open(sys.argv[8]+".s","w").write(outputConstantsS(l))
open(sys.argv[8]+".py","w").write(outputConstantsPY(l))
