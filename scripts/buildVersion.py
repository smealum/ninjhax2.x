import sys
import os
import re

def getRoVersion(v):
	if v[0]<4:
		return "0"
	elif v[0]<5:
		return "1024"
	elif not(v[0]>=7 and v[1]>=2) and v[0]<=7:
		return "2049"
	elif v[0]<8:
		return "3074"
	else:
		return "4096"

MENU_VERSION_MAP = {
	9: {
		0: {"E": "11272", "U": "11272", "J": "11272", "K": None},
		1: {"E": "11272", "U": "11272", "J": "11272", "K": None},
		2: {"E": "12288", "U": "12288", "J": "12288", "K": None},
		3: {"E": "13330", "U": "13330", "J": "13330", "K": None},
		4: {"E": "14336", "U": "14336", "J": "14336", "K": None},
		5: {"E": "15360", "U": "15360", "J": "15360", "K": None},
		6: {"E": "16404", "U": "16404", "J": "16404", "K": "6166_kor"},
		7: {"E": "17415", "U": "17415", "J": "17415", "K": "7175_kor"},
		8: {"E": "19456", "U": "19456", "J": "19456", "K": "7175_kor"},
		9: {"E": "19456", "U": "20480_usa", "J": "19456", "K": "7175_kor"},
	},
	10: {
		0: {"E": "19456", "U": "20480_usa", "J": "19456", "K": "7175_kor"},
		1: {"E": "20480", "U": "21504_usa", "J": "20480", "K": "8192_kor"},
		2: {"E": "21504", "U": "22528_usa", "J": "21504", "K": "9216_kor"},
		3: {"E": "22528", "U": "23552_usa", "J": "22528", "K": "10240_kor"},
		4: {"E": "23554", "U": "24578_usa", "J": "23554", "K": "11266_kor"},
		5: {"E": "23554", "U": "24578_usa", "J": "23554", "K": "11266_kor"},
		6: {"E": "24576", "U": "25600_usa", "J": "24576", "K": "12288_kor"},
		7: {"E": "24576", "U": "25600_usa", "J": "24576", "K": "12288_kor"},
	},
	11: {
		0: {"E": "24576", "U": "25600_usa", "J": "24576", "K": "12288_kor"},
		1: {"E": "25600", "U": "26624_usa", "J": "25600", "K": "13312_kor"},
		2: {"E": "25600", "U": "26624_usa", "J": "25600", "K": "13312_kor"},
		3: {"E": "26624", "U": "27648_usa", "J": "26624", "K": "14336_kor"},
		4: {"E": "26624", "U": "27648_usa", "J": "26624", "K": "14336_kor"},
	},
}

def getMenuVersion(v):
	try:
		return MENU_VERSION_MAP[v[0]][v[1]][v[4]] or "unsupported"
	except KeyError:
		return "unsupported"

def getMsetVersion(v):
	if v[0] == 9 and v[1] < 6:
		return "8203"
	else:
		return "9221"

def getRegion(v):
	return v[4]

def getFirmVersion(v):
	if v[5]==1:
		return "N3DS"
	else:
		return "POST5"

def getNeedsUdsploit(v):
	return (getFirmVersion(v) != "N3DS" and v[0] >= 11 and v[1] == 3)

if __name__ == '__main__':
	#format : "X.X.X-XR"
	version=sys.argv[1]
	p=re.compile("^([N]?)([0-9]+)\.([0-9]+)\.([0-9]+)-([0-9]+)([EUJK])")
	r=p.match(version)

	if r:
		new3DS=(1 if (r.group(1)=="N") else 0)
		cverMajor=int(r.group(2))
		cverMinor=int(r.group(3))
		cverMicro=int(r.group(4))
		nupVersion=int(r.group(5))
		nupRegion=r.group(6)
		extraparams=""
		extraparams+=" LOADROPBIN=1"
		qrinstaller = False
		for arg in sys.argv:
			# if(arg=="--enableloadropbin"):
			# 	extraparams+=" LOADROPBIN=1"
			if(arg=="--enableqrinstaller"):
				qrinstaller = True
				extraparams+=" QRINSTALLER=1"
			if(arg=="--enableotherapp"):
				extraparams+=" OTHERAPP=1"
			if(arg=="--enablerecovery"):
				extraparams+=" RECOVERY=1"
		v=(cverMajor, cverMinor, cverMicro, nupVersion, nupRegion, new3DS)
		if getNeedsUdsploit(v):
			extraparams += " UDSPLOIT=1"
		os.system("make clean")
		os.system("make REGION="+getRegion(v)+" ROVERSION="+getRoVersion(v)+" MSETVERSION="+getMsetVersion(v)+" FIRMVERSION="+getFirmVersion(v)+" MENUVERSION="+getMenuVersion(v)+extraparams)
		if qrinstaller:
			os.system("python scripts/buildQrInstaller.py %s_%s_%s_%s" % (getFirmVersion(v), getRegion(v), getMenuVersion(v), getMsetVersion(v)))
	else:
		print("invalid version format; learn2read.")
