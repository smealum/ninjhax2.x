static const u32 processLinearOffset[] =
{
	0x00300000, // camera app
	0x000B0000, // dlplay app
	0x00300000, // act app
};

static const u32 processHookAddress[] =
{
	0x00104be0, // camera app
	0x00100D00, // dlplay app
	0x00100160, // act app
};

static const u32 processHookTidLow[] =
{
	CAMAPP_TIDLOW, // camera app
	DLPLAY_TIDLOW, // dlplay app
	ACTAPP_TIDLOW, // act app
};

typedef struct {
	u32 num;
	u32 text_end;
	u32 data_address;
	u32 data_size;
	bool capabilities[2]; // {socuAccess, csndAccess}
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
		{false, true},
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
		{true, true},
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
		{true, false},
		{
			{0x00100000, 0x00008000, 0x00300000 - 0x00008000},
			{0x00100000 + 0x00300000 - 0x00008000, - 0x0000E000, 0x0000E000},
			{0x00100000 + 0x00300000 + 0x0000E000 - 0x00008000, - 0x00010000, 0x00002000},
			{0x00100000 + 0x00300000 + 0x0000E000 + 0x00002000 - 0x00008000, - 0x00070000, 0x00060000},
		}
	};

static const memorymap_t * const app_maps[] =
	{
		(memorymap_t*)&camapp_map, // camera app
		(memorymap_t*)&dlplay_map, // dlplay app
		(memorymap_t*)&actapp_map, // act app
	};

static const int numTargetProcesses = 3;

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
