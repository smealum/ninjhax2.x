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
regionVersions=["E", "U", "J"]
msetVersions=[8203, 9221]
roVersions=[4096]
menuVersions=[11272, 12288, 13330, 15360, 16404, 17415, 19456, "20480_usa"]

a=[firmVersions, regionVersions, msetVersions, roVersions, menuVersions]

extraparams=""
extraparams+=" LOADROPBIN=1"
for arg in sys.argv:
	# if(arg=="--enableloadropbin"):
		# extraparams+=" LOADROPBIN=1"
	if(arg=="--enableotherapp"):
		extraparams+=" OTHERAPP=1"

cnt=0
for v in (list(itertools.product(*a))):
	if isVersionPossible(v):
		os.system("make clean")	
		os.system("make FIRMVERSION="+str(v[0])+" REGION="+str(v[1])+" MSETVERSION="+str(v[2])+" ROVERSION="+str(v[3])+" MENUVERSION="+str(v[4])+extraparams)
print(cnt)
