ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

ifeq ($(strip $(CTRULIB)),)
$(error "Please set CTRULIB in your environment. export DEVKITARM=<path to>ctrulib/libctru")
endif

ifeq ($(filter $(DEVKITARM)/bin,$(PATH)),)
export PATH:=$(DEVKITARM)/bin:$(PATH)
endif

ifeq ($(REGION), J)
CNVERSION = JPN
else
CNVERSION = WEST
endif

export FIRMVERSION
export CNVERSION
export REGION
export ROVERSION
export MSETVERSION
export MENUVERSION
export LOADROPBIN

ROPBIN_CMD0	:=	
ROPBIN_CMD1	:=	
ifneq ($(strip $(LOADROPBIN)),)
	ROPBIN_CMD0	:=	@cp build/menu_ropbin.bin cn_secondary_payload/data/
	ROPBIN_CMD1	:=	@cp menu_payload/menu_ropbin.bin build/
endif

ROPDB_VERSIONS = 11272 12288 13330 14336 15360 16404 17415 19456 20480_usa
ROPDB_TARGETS = $(addsuffix _ropdb.txt, $(addprefix menu_ropdb/, $(ROPDB_VERSIONS)))

OUTNAME = $(FIRMVERSION)_$(REGION)_$(MENUVERSION)_$(MSETVERSION)

SCRIPTS = "scripts"

.PHONY: directories all menu_ropdb build/constants firm_constants/constants.txt cn_constants/constants.txt region_constants/constants.txt menu_ropdb/ropdb.txt cn_qr_initial_loader/cn_qr_initial_loader.bin.png cn_save_initial_loader/cn_save_initial_loader.bin cn_secondary_payload/cn_secondary_payload.bin cn_bootloader/cn_bootloader.bin menu_payload/menu_payload_regionfree.bin menu_payload/menu_payload_loadropbin.bin menu_payload/menu_ropbin.bin

all: directories build/constants q/$(OUTNAME).png p/$(OUTNAME).bin build/cn_save_initial_loader.bin
directories:
	@mkdir -p build && mkdir -p build/cro
	@mkdir -p p
	@mkdir -p q

menu_ropdb: $(ROPDB_TARGETS)

menu_ropdb/%_ropdb.txt: menu_ropdb/17415_ropdb_proto.txt
	@echo building ropDB for menu version $*...
	@python scripts/portRopDb.py menu_17415_code.bin menu_$*_code.bin 0x00100000 menu_ropdb/17415_ropdb_proto.txt menu_ropdb/$*_ropdb.txt

q/$(OUTNAME).png: build/cn_qr_initial_loader.bin.png
	@cp build/cn_qr_initial_loader.bin.png q/$(OUTNAME).png

p/$(OUTNAME).bin: build/cn_secondary_payload.bin
	@cp build/cn_secondary_payload.bin p/$(OUTNAME).bin

firm_constants/constants.txt:
	@cd firm_constants && make
cn_constants/constants.txt:
	@cd cn_constants && make
region_constants/constants.txt:
	@cd region_constants && make
menu_ropdb/ropdb.txt:
	@cd menu_ropdb && make

build/constants: firm_constants/constants.txt cn_constants/constants.txt region_constants/constants.txt menu_ropdb/ropdb.txt
	@python $(SCRIPTS)/makeHeaders.py $(FIRMVERSION) $(CNVERSION) $(MSETVERSION) $(ROVERSION) $(MENUVERSION) $(REGION) $(OUTNAME) build/constants $^

build/cn_qr_initial_loader.bin.png: cn_qr_initial_loader/cn_qr_initial_loader.bin.png
	@cp cn_qr_initial_loader/cn_qr_initial_loader.bin.png build
cn_qr_initial_loader/cn_qr_initial_loader.bin.png:
	@cd cn_qr_initial_loader && make


build/cn_save_initial_loader.bin: cn_save_initial_loader/cn_save_initial_loader.bin
	@cp cn_save_initial_loader/cn_save_initial_loader.bin build
cn_save_initial_loader/cn_save_initial_loader.bin:
	@cd cn_save_initial_loader && make


build/app_code.bin: app_code/app_code.bin
	@cp app_code/app_code.bin build
build/app_code_reloc.s: app_code/app_code_reloc.s
	@cp app_code/app_code_reloc.s build
app_code/app_code.bin app_code/app_code_reloc.s:
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
menu_payload/menu_payload_regionfree.bin menu_payload/menu_payload_loadropbin.bin menu_payload/menu_ropbin.bin: build/app_code.bin build/app_code_reloc.s build/app_bootloader.bin
	@cp build/app_bootloader.bin menu_payload/
	@cp build/app_code.bin menu_payload/
	@cp build/app_code_reloc.s menu_payload/
	@cd menu_payload && make


clean:
	@rm -rf build/*
	@cd firm_constants && make clean
	@cd cn_constants && make clean
	@cd region_constants && make clean
	@cd menu_ropdb && make clean
	@cd cn_qr_initial_loader && make clean
	@cd cn_save_initial_loader && make clean
	@cd cn_secondary_payload && make clean
	@cd menu_payload && make clean
	@cd app_bootloader && make clean
	@cd app_code && make clean
	@echo "all cleaned up !"
