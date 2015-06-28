ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

ifeq ($(strip $(CTRULIB)),)
$(error "Please set CTRULIB in your environment. export DEVKITARM=<path to>ctrulib/libctru")
endif

ifeq ($(filter $(DEVKITARM)/bin,$(PATH)),)
export PATH:=$(DEVKITARM)/bin:$(PATH)
endif

# FIRMVERSION = OLD_MEMMAP
# FIRMVERSION = NEW_MEMMAP

# CNVERSION = WEST
# CNVERSION = JPN
# ROVERSION = 1024
# ROVERSION = 2049
# ROVERSION = 3074
# ROVERSION = 4096
# SPIDERVERSION = 2050
# SPIDERVERSION = 3074
# SPIDERVERSION = 4096

export FIRMVERSION
export CNVERSION
export ROVERSION
export SPIDERVERSION
export MENUVERSION
export LOADROPBIN

ROPBIN_CMD0	:=	
ROPBIN_CMD1	:=	
ifneq ($(strip $(LOADROPBIN)),)
	ROPBIN_CMD0	:=	@cp build/menu_ropbin.bin cn_secondary_payload/data/
	ROPBIN_CMD1	:=	@cp menu_payload/menu_ropbin.bin build/
endif

OUTNAME = $(FIRMVERSION)_$(CNVERSION)_$(MENUVERSION)

SCRIPTS = "scripts"

.PHONY: directories all menu_ropdb build/constants firm_constants/constants.txt cn_constants/constants.txt menu_ropdb/ropdb.txt cn_qr_initial_loader/cn_qr_initial_loader.bin.png cn_save_initial_loader/cn_save_initial_loader.bin cn_secondary_payload/cn_secondary_payload.bin cn_bootloader/cn_bootloader.bin menu_payload/menu_payload_regionfree.bin menu_payload/menu_payload_loadropbin.bin menu_payload/menu_ropbin.bin

all: directories build/constants q/$(OUTNAME).png p/$(OUTNAME).bin build/cn_save_initial_loader.bin
directories:
	@mkdir -p build && mkdir -p build/cro
	@mkdir -p p
	@mkdir -p q

menu_ropdb:
	@mkdir -p menu_ropdb/11272
	@mkdir -p menu_ropdb/12288
	@mkdir -p menu_ropdb/13330
	@mkdir -p menu_ropdb/15360
	@mkdir -p menu_ropdb/16404
	@mkdir -p menu_ropdb/17415
	@mkdir -p menu_ropdb/19456
	@echo building ropDB for menu version 11272...
	@python scripts/portRopDb.py menu_17415_code.bin menu_11272_code.bin 0x00100000 menu_ropdb/17415_proto/ropdb.txt menu_ropdb/11272/ropdb.txt
	@echo building ropDB for menu version 12288...
	@python scripts/portRopDb.py menu_17415_code.bin menu_12288_code.bin 0x00100000 menu_ropdb/17415_proto/ropdb.txt menu_ropdb/12288/ropdb.txt
	@echo building ropDB for menu version 13330...
	@python scripts/portRopDb.py menu_17415_code.bin menu_13330_code.bin 0x00100000 menu_ropdb/17415_proto/ropdb.txt menu_ropdb/13330/ropdb.txt
	@echo building ropDB for menu version 15360...
	@python scripts/portRopDb.py menu_17415_code.bin menu_15360_code.bin 0x00100000 menu_ropdb/17415_proto/ropdb.txt menu_ropdb/15360/ropdb.txt
	@echo building ropDB for menu version 16404...
	@python scripts/portRopDb.py menu_17415_code.bin menu_16404_code.bin 0x00100000 menu_ropdb/17415_proto/ropdb.txt menu_ropdb/16404/ropdb.txt
	@echo building ropDB for menu version 17415...
	@python scripts/portRopDb.py menu_17415_code.bin menu_17415_code.bin 0x00100000 menu_ropdb/17415_proto/ropdb.txt menu_ropdb/17415/ropdb.txt
	@echo building ropDB for menu version 19456...
	@python scripts/portRopDb.py menu_17415_code.bin menu_19456_code.bin 0x00100000 menu_ropdb/17415_proto/ropdb.txt menu_ropdb/19456/ropdb.txt

q/$(OUTNAME).png: build/cn_qr_initial_loader.bin.png
	@cp build/cn_qr_initial_loader.bin.png q/$(OUTNAME).png

p/$(OUTNAME).bin: build/cn_secondary_payload.bin
	@cp build/cn_secondary_payload.bin p/$(OUTNAME).bin

firm_constants/constants.txt:
	@cd firm_constants && make
cn_constants/constants.txt:
	@cd cn_constants && make
menu_ropdb/ropdb.txt:
	@cd menu_ropdb && make

build/constants: firm_constants/constants.txt cn_constants/constants.txt menu_ropdb/ropdb.txt
	@python $(SCRIPTS)/makeHeaders.py $(FIRMVERSION) $(CNVERSION) $(SPIDERVERSION) $(ROVERSION) $(MENUVERSION) build/constants $^

build/cn_qr_initial_loader.bin.png: cn_qr_initial_loader/cn_qr_initial_loader.bin.png
	@cp cn_qr_initial_loader/cn_qr_initial_loader.bin.png build
cn_qr_initial_loader/cn_qr_initial_loader.bin.png:
	@cd cn_qr_initial_loader && make


build/cn_save_initial_loader.bin: cn_save_initial_loader/cn_save_initial_loader.bin
	@cp cn_save_initial_loader/cn_save_initial_loader.bin build
cn_save_initial_loader/cn_save_initial_loader.bin:
	@cd cn_save_initial_loader && make


build/sns_code.bin: sns_code/sns_code.bin
	@cp sns_code/sns_code.bin build
sns_code/sns_code.bin:
	@cd sns_code && make


build/app_code.bin: app_code/app_code.bin
	@cp app_code/app_code.bin build
app_code/app_code.bin: build/app_bootloader.bin
	@mkdir -p app_code/data
#	@cp build/app_bootloader.bin app_code/data/
	@cd app_code && make


build/app_bootloader.bin: app_bootloader/app_bootloader.bin
	@cp app_bootloader/app_bootloader.bin build
app_bootloader/app_bootloader.bin:
	@cd app_bootloader && make


build/cn_secondary_payload.bin: cn_secondary_payload/cn_secondary_payload.bin
	@python $(SCRIPTS)/blowfish.py cn_secondary_payload/cn_secondary_payload.bin build/cn_secondary_payload.bin scripts
cn_secondary_payload/cn_secondary_payload.bin: build/cn_save_initial_loader.bin build/menu_payload_regionfree.bin build/menu_payload_loadropbin.bin build/menu_ropbin.bin
	@mkdir -p cn_secondary_payload/data
	@cp build/cn_save_initial_loader.bin cn_secondary_payload/data/
	@cp build/menu_payload_regionfree.bin cn_secondary_payload/data/
	@cp build/menu_payload_loadropbin.bin cn_secondary_payload/data/
	$(ROPBIN_CMD0)
	@cd cn_secondary_payload && make


build/menu_payload_regionfree.bin build/menu_payload_loadropbin.bin build/menu_ropbin.bin: menu_payload/menu_payload_regionfree.bin menu_payload/menu_payload_loadropbin.bin menu_payload/menu_ropbin.bin
	@cp menu_payload/menu_payload_regionfree.bin build/
	@cp menu_payload/menu_payload_loadropbin.bin build/
	$(ROPBIN_CMD1)
menu_payload/menu_payload_regionfree.bin menu_payload/menu_payload_loadropbin.bin menu_payload/menu_ropbin.bin: build/sns_code.bin build/app_code.bin
	@cp build/sns_code.bin menu_payload/
	@cp build/app_code.bin menu_payload/
	@cd menu_payload && make


clean:
	@rm -rf build/*
	@cd firm_constants && make clean
	@cd cn_constants && make clean
	@cd menu_ropdb && make clean
	@cd cn_qr_initial_loader && make clean
	@cd cn_save_initial_loader && make clean
	@cd cn_secondary_payload && make clean
	@cd menu_payload && make clean
	@cd app_bootloader && make clean
	@cd app_code && make clean
	@cd sns_code && make clean
	@echo "all cleaned up !"
