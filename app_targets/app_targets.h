typedef struct
{
	u32 num;
	u32 text_end;
	u32 data_address;
	u32 data_size;
	u32 processLinearOffset;
	u32 processHookAddress;
	u32 processAppCodeAddress;
	u32 processHookTidLow, processHookTidHigh;
	u32 mediatype;
	bool capabilities[0x10]; // {socuAccess, csndAccess, qtmAccess, nfcAccess, httpcAccess, reserved...}
} memorymap_header_t;

typedef struct
{
	u32 src, dst, size;
} memorymap_entry_t;

typedef struct {
	memorymap_header_t header;
	memorymap_entry_t map[];
} memorymap_t;

#define size_memmap(m) (sizeof(memorymap_header_t) + sizeof(memorymap_entry_t) * (m).header.num)

#define NUM_CAPABILITIES 5

static const memorymap_t camapp_map =
	{
		{
			4,
			0x00347000,
			0x00429000,
			0x00046680 + 0x00099430,
			0x00300000, // processLinearOffset
			0x00104be0, // processHookAddress
			0x00105000, // processAppCodeAddress
			CAMAPP_TIDLOW, 0x00040010, 0x0,
			{false, true, false, false, false,
				false, false, false, false, false, false, false, false, false, false, false},
			},
		{
			{0x00100000, 0x00000000, 0x00300000},
			{0x00100000 + 0x00300000, - 0x00070000, 0x00070000},
			{0x00100000 + 0x00300000 + 0x00070000, - 0x00100000, 0x00090000},
			{0x00100000 + 0x00300000 + 0x00070000 + 0x00090000, - 0x00109000, 0x00009000},
		}
	};

static const memorymap_t dlplay_map =
	{
		{
			4,
			0x00193000,
			0x001A0000,
			0x00013790 + 0x0002A538,
			0x000B0000, // processLinearOffset
			0x00100D00, // processHookAddress
			0x00105000, // processAppCodeAddress
			DLPLAY_TIDLOW, 0x00040010, 0x0,
			{true, true, false, false, true,
				false, false, false, false, false, false, false, false, false, false, false},
			},
		{
			{0x100000, 0x0000, 0xb0000},
			{0x1b0000, -0x4000, 0x4000},
			{0x1b4000, -0x2e000, 0xc000},
			{0x1c0000, -0x20000, 0x1c000},
			{0x1dc000, -0x22000, 0x2000},
		}
	};

static const memorymap_t actapp_map =
	{
		{
			4,
			0x00388000,
			0x003F3000,
			0x0001A2FC + 0x00061ED4,
			0x00300000, // processLinearOffset
			0x00100160, // processHookAddress
			0x00105000, // processAppCodeAddress
			ACTAPP_TIDLOW, 0x00040010, 0x0,
			{true, false, false, false, false,
				false, false, false, false, false, false, false, false, false, false, false},
			},
		{
			{0x00100000, 0x00000000, 0x00300000},
			{0x00100000 + 0x00300000, - 0x0000E000, 0x0000E000},
			{0x00100000 + 0x00300000 + 0x0000E000, - 0x00010000, 0x00002000},
			{0x00100000 + 0x00300000 + 0x0000E000 + 0x00002000, - 0x00070000, 0x00060000},
		}
	};


static const memorymap_t msetapp_map =
	#if (MSET_VERSION == 8203)
		// 9.0-9.6
		{
			{
				8,
				0x00268000,
				0x00291000,
				0x00017B98 + 0x004EB108,
				0x00100000, // processLinearOffset
				0x00101530, // processHookAddress
				0x00105000, // processAppCodeAddress
				MSET_TIDLOW, 0x00040010, 0x0,
				{true, false, true, false, true,
					false, false, false, false, false, false, false, false, false, false, false},
			},
			{
				{0x100000,  0x0000, 0x100000},
				{0x200000, -0xa0000, 0xa0000},
				{0x2a0000, -0xa9000, 0x9000},
				{0x2a9000, -0xac000, 0x3000},
				{0x2ac000, -0x594000, 0x54000},
				{0x300000, -0x500000, 0x450000},
				{0x750000, -0x540000, 0x40000},
				{0x790000, -0xb0000, 0x4000},
			}
		};
	#else
		// 9.6+
		{
			{
				9,
				0x0026A000,
				0x00293000,
				0x00017BA0 + 0x004EB100,
				0x00100000, // processLinearOffset
				0x00101480, // processHookAddress
				0x00105000, // processAppCodeAddress
				MSET_TIDLOW, 0x00040010, 0x0,
				{true, false, true, false, true,
					false, false, false, false, false, false, false, false, false, false, false},
			},
			{
				{0x100000, 0x0000, 0x100000},
				{0x200000, -0xa0000, 0xa0000},
				{0x2a0000, -0xab000, 0xb000},
				{0x2ab000, -0x596000, 0x5000},
				{0x2b0000, -0x590000, 0x50000},
				{0x300000, -0x500000, 0x450000},
				{0x750000, -0x540000, 0x40000},
				{0x790000, -0xb0000, 0x5000},
				{0x795000, -0x591000, 0x1000},
			}
		};
	#endif

static const memorymap_t nfaceapp_map =
	{
		{
			6,
			0x003C6000,
			0x004C3000,
			0x0003AA8C + 0x000AE4C4,
			0x00300000, // processLinearOffset
			0x001001A0, // processHookAddress
			0x00105000, // processAppCodeAddress
			NFACE_TIDLOW, 0x00040010, 0x0,
			{true, false, false, true, false,
				false, false, false, false, false, false, false, false, false, false, false},
		},
		{
			{0x100000, 0x0000, 0x300000},
			{0x400000, -0xf0000, 0xf0000},
			{0x4f0000, -0xfe000, 0xe000},
			{0x4fe000, -0x1ac000, 0x2000},
			{0x500000, -0x1a0000, 0xa2000},
			{0x5a2000, -0x1aa000, 0xa000},
		}
	};

static const memorymap_t * const app_maps[] =
	{
		(memorymap_t*)&camapp_map, // camera app
		(memorymap_t*)&dlplay_map, // dlplay app
		(memorymap_t*)&actapp_map, // act app
		(memorymap_t*)&msetapp_map, // mset app
		(memorymap_t*)&nfaceapp_map, // n3ds FACE app
	};

// IS_N3DS is used to add n3ds-specific targets
// this way i only need to have the define in one location instead of #ifdefs everywhere
static const int numTargetProcesses = 4 + IS_N3DS;

static void patchPayload(u32* payload_dst, int targetProcessIndex, memorymap_t* mmap)
{
	if(!mmap) mmap = (memorymap_t*)app_maps[targetProcessIndex];
	// payload has a bunch of aliases to handle multiple target processes "gracefully"
	int i;
	for(i = 0; i < 0x10000 / 4; i++)
	{
		u32 val = payload_dst[i];
		if(val >> 16 == 0xBABE)
		{
			val &= 0xFFFF;
				// target process index
				if(val == 0x0001)
				{
					payload_dst[i] = targetProcessIndex;
				}
				// target process APP_START_LINEAR
				else if(val == 0x0002)
				{
					payload_dst[i] = 0x30000000 + FIRM_APPMEMALLOC - mmap->header.processLinearOffset;
				}
				// target process hook virtual address
				else if(val == 0x0003)
				{
					payload_dst[i] = mmap->header.processHookAddress;
				}
				// target process TID low (for nss_launchtitle)
				else if(val == 0x0004)
				{
					if(mmap->header.mediatype == 2) payload_dst[i] = 0;
					else payload_dst[i] = mmap->header.processHookTidLow;
				}
				// target process TID high (for nss_launchtitle)
				else if(val == 0x0005)
				{
					if(mmap->header.mediatype == 2) payload_dst[i] = 0;
					else payload_dst[i] = mmap->header.processHookTidHigh;
				}
				// cur process map
				else if(val == 0x0006)
				{
					memcpy(&payload_dst[i], mmap, sizeof(memorymap_header_t) + sizeof(memorymap_entry_t) * mmap->header.num);
				}
				// app_code base address
				else if(val == 0x0007)
				{
					payload_dst[i] = mmap->header.processAppCodeAddress;
				}
				// target process TID low (for things other than nss_launchtitle)
				else if(val == 0x0008)
				{
					payload_dst[i] = mmap->header.processHookTidLow & ~0x20000000; // mask for nss_terminate
				}
				// target process TID high (for things other than nss_launchtitle)
				else if(val == 0x0009)
				{
					payload_dst[i] = mmap->header.processHookTidHigh;
				}
				else if(val == 0x000A)
				{
					payload_dst[i] = mmap->header.mediatype;
				}
		}
	}
}
