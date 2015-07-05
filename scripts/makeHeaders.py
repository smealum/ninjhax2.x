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

if len(sys.argv)<7:
	print("use : "+sys.argv[0]+" <firmver> <cnver> <spiderver> <rover> <menuver> <region> <extensionless_output_name> <input_file1> <input_file2> ...")
	exit()

# l=[("_SPIDER_VERSION", sys.argv[3]),
l=[("_RO_VERSION", sys.argv[4])]
l+=[("FIRM_VERSION", "\""+sys.argv[1]+"\""),
	("CN_VERSION", "\""+sys.argv[2]+"\""),
	("SPIDER_VERSION", "\""+sys.argv[3]+"\""),
	("RO_VERSION", "\""+sys.argv[4]+"\""),
	("MENU_VERSION", "\""+sys.argv[5]+"\""),
	("REGION", "\""+sys.argv[6]+"\"")]
l+=[("BUILDTIME", "\""+datetime.now().strftime("%Y-%m-%d %H:%M:%S")+"\"")]
l+=[("HB_NUM_HANDLES", "16")]

for fn in sys.argv[8:]:
	s=open(fn,"r").read()
	if len(s)>0:
		l+=(ast.literal_eval(s))

open(sys.argv[7]+".h","w").write(outputConstantsH(l))
open(sys.argv[7]+".s","w").write(outputConstantsS(l))
open(sys.argv[7]+".py","w").write(outputConstantsPY(l))
