import sys
import os
import itertools

# 0 : firm, 1 : cn, 2 : spider, 3 : ro

def isVersionPossible(v):
	if v[0]=="PRE5":
		return v[3]<=1024 and v[2]<=2050
	else:
		return v[3]>=2049

firmVersions=["POST5", "N3DS"]
cnVersions=["WEST", "JPN"]
spiderVersions=[4096]
roVersions=[4096]
menuVersions=[12288, 17415]

a=[firmVersions, cnVersions, spiderVersions, roVersions, menuVersions]

loadropbin=""
if(len(sys.argv)>=2 and sys.argv[1]=="--enableloadropbin"):
	loadropbin=" LOADROPBIN=1"

cnt=0
for v in (list(itertools.product(*a))):
	if isVersionPossible(v):
		os.system("make clean")	
		os.system("make FIRMVERSION="+str(v[0])+" CNVERSION="+str(v[1])+" SPIDERVERSION="+str(v[2])+" ROVERSION="+str(v[3])+" MENUVERSION="+str(v[4])+loadropbin)
print(cnt)
