static const u32 processLinearOffset[] =
{
	0x00300000, // camera app
	0x000B0000, // dlplay app
	0x00300000, // act app
	0x00100000, // mset app
};

static const u32 processHookAddress[] =
{
	0x00104be0, // camera app
	0x00100D00, // dlplay app
	0x00100160, // act app
	#if (MSET_VERSION == 8203)
		0x00101530, // mset app (9.0 - 9.6)
	#else
		0x00101480, // mset app (9.6+)
	#endif
};

static const u32 processHookTidLow[] =
{
	CAMAPP_TIDLOW, // camera app
	DLPLAY_TIDLOW, // dlplay app
	ACTAPP_TIDLOW, // act app
	MSET_TIDLOW, // mset app
};

typedef struct {
	u32 num;
	u32 text_end;
	u32 data_address;
	u32 data_size;
	bool capabilities[0x10]; // {socuAccess, csndAccess, qtmAccess, reserved}
	struct {
		u32 src, dst, size;
	} map[];
} memorymap_t;

static const memorymap_t camapp_map =
	{
		4,
		0x00347000,
		0x00429000,
		0x00046680 + 0x00099430,
		{false, true, false,
			false, false, false, false, false, false, false, false, false, false, false, false, false},
		{
			{0x00100000, 0x00008000, 0x00300000 - 0x00008000},
			{0x00100000 + 0x00300000 - 0x00008000, - 0x00070000, 0x00070000},
			{0x00100000 + 0x00300000 + 0x00070000 - 0x00008000, - 0x00100000, 0x00090000},
			{0x00100000 + 0x00300000 + 0x00070000 + 0x00090000 - 0x00008000, - 0x00109000, 0x00009000},
		}
	};

static const memorymap_t dlplay_map =
	{
		4,
		0x00193000,
		0x001A0000,
		0x00013790 + 0x0002A538,
		{true, true, false,
			false, false, false, false, false, false, false, false, false, false, false, false, false},
		{
			{0x00100000, 0x00008000, 0x000B0000 - 0x00008000},
			{0x00100000 + 0x000B0000 - 0x00008000, - 0x000B4000, 0x00004000},
			{0x00100000 + 0x000B4000 - 0x00008000, - 0x000DE000, 0x0002A000},
			{0x00100000 + 0x000B4000 + 0x0002A000 - 0x00008000, - 0x000D2000, 0x00002000},
		}
	};

static const memorymap_t actapp_map =
	{
		4,
		0x00388000,
		0x003F3000,
		0x0001A2FC + 0x00061ED4,
		{true, false, false,
			false, false, false, false, false, false, false, false, false, false, false, false, false},
		{
			{0x00100000, 0x00008000, 0x00300000 - 0x00008000},
			{0x00100000 + 0x00300000 - 0x00008000, - 0x0000E000, 0x0000E000},
			{0x00100000 + 0x00300000 + 0x0000E000 - 0x00008000, - 0x00010000, 0x00002000},
			{0x00100000 + 0x00300000 + 0x0000E000 + 0x00002000 - 0x00008000, - 0x00070000, 0x00060000},
		}
	};


static const memorymap_t msetapp_map =
	#if (MSET_VERSION == 8203)
		// 9.2-9.6
		{
			8,
			0x00268000,
			0x00291000,
			0x00017B98 + 0x004EB108,
			{true, false, true,
				false, false, false, false, false, false, false, false, false, false, false, false, false},
			{
				{0x100000,  0x8000, 0x100000 - 0x8000},
				{0x200000 - 0x8000, -0xa0000, 0xa0000},
				{0x2a0000 - 0x8000, -0xa9000, 0x9000},
				{0x2a9000 - 0x8000, -0xac000, 0x3000},
				{0x2ac000 - 0x8000, -0x594000, 0x54000},
				{0x300000 - 0x8000, -0x500000, 0x450000},
				{0x750000 - 0x8000, -0x540000, 0x40000},
				{0x790000 - 0x8000, -0xb0000, 0x4000},
			}
		};
	#else
		// 9.6+
		{
			9,
			0x0026A000,
			0x00293000,
			0x00017BA0 + 0x004EB100,
			{true, false, true,
				false, false, false, false, false, false, false, false, false, false, false, false, false},
			{
				{0x100000, 0x8000, 0x100000 - 0x8000},
				{0x200000 - 0x8000, -0xa0000, 0xa0000},
				{0x2a0000 - 0x8000, -0xab000, 0xb000},
				{0x2ab000 - 0x8000, -0x596000, 0x5000},
				{0x2b0000 - 0x8000, -0x590000, 0x50000},
				{0x300000 - 0x8000, -0x500000, 0x450000},
				{0x750000 - 0x8000, -0x540000, 0x40000},
				{0x790000 - 0x8000, -0xb0000, 0x5000},
				{0x795000 - 0x8000, -0x591000, 0x1000},
			}
		};
	#endif

static const memorymap_t * const app_maps[] =
	{
		(memorymap_t*)&camapp_map, // camera app
		(memorymap_t*)&dlplay_map, // dlplay app
		(memorymap_t*)&actapp_map, // act app
		(memorymap_t*)&msetapp_map, // mset app
	};

static const int numTargetProcesses = 4;

static void patchPayload(u32* payload_dst, int targetProcessIndex)
{
	// payload has a bunch of aliases to handle multiple target processes "gracefully"
	int i;
	for(i=0; i<0x10000/4; i++)
	{
		switch(payload_dst[i])
		{
			// target process index
			case 0xBABE0001:
				payload_dst[i] = targetProcessIndex;
				break;
			// target process APP_START_LINEAR
			case 0xBABE0002:
				payload_dst[i] = 0x30000000 + FIRM_APPMEMALLOC - processLinearOffset[targetProcessIndex];
				break;
			// target process hook virtual address
			case 0xBABE0003:
				payload_dst[i] = processHookAddress[targetProcessIndex];
				break;
			// target process TID low
			case 0xBABE0004:
				payload_dst[i] = processHookTidLow[targetProcessIndex];
				break;
		}
	}
}
