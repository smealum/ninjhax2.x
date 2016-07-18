import sys
import os
import itertools
import buildVersion

# 0 : firm, 1 : cn, 2 : spider, 3 : ro

firmVersions=["POST5", "N3DS"]

extraparams=""
extraparams+=" LOADROPBIN=1"
for arg in sys.argv:
	# if(arg=="--enableloadropbin"):
		# extraparams+=" LOADROPBIN=1"
	if(arg=="--enableotherapp"):
		extraparams+=" OTHERAPP=1"
	if(arg=="--enablerecovery"):
		extraparams+=" RECOVERY=1"

supportVersions = []
for v_1, _ in buildVersion.MENU_VERSION_MAP.items():
	for v_2, __ in _.items():
		mset = buildVersion.getMsetVersion((v_1, v_2))
		ro = buildVersion.getRoVersion((v_1, v_2))
		for region, menu in __.items():
			if buildVersion.getMenuVersion((v_1, v_2, None, None, region)) == 'unsupported':
				continue
			for firm in firmVersions:
				v = (firm, region, mset, ro, menu)
				if v in supportVersions:
					continue
				supportVersions.append(v)

cnt=0
for v in supportVersions:
	os.system("make clean")
	os.system("make FIRMVERSION="+str(v[0])+" REGION="+str(v[1])+" MSETVERSION="+str(v[2])+" ROVERSION="+str(v[3])+" MENUVERSION="+str(v[4])+extraparams)
print(cnt)
