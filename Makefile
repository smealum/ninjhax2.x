ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif
export CTRULIB=$(shell pwd)/libctru
ifeq ($(strip $(CTRULIB)),)
$(error "Please set CTRULIB in your environment. export CTRULIB=<path to>ctrulib/libctru")
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
export OTHERAPP
export UDSPLOIT
export QRINSTALLER

PAYLOAD_SRCPATH	:=	build/cn_secondary_payload.bin

ROPBIN_CMD0	:=	
ROPBIN_CMD1	:=	
ifneq ($(strip $(LOADROPBIN)),)
	ROPBIN_CMD0	:=	@compress/compress.exe build/menu_ropbin.bin cn_secondary_payload/data/menu_ropbin.bin
	ROPBIN_CMD1	:=	@cp menu_payload/menu_ropbin.bin build/
endif

ROPDB_VERSIONS = 11272 12288 13330 14336 15360 16404 17415 19456 20480 21504 22528 23554 24576 25600 26624 20480_usa 21504_usa 22528_usa 23552_usa 24578_usa 25600_usa 26624_usa 27648_usa 6166_kor 7175_kor 8192_kor 9216_kor 10240_kor 11266_kor 12288_kor 13312_kor 14336_kor
ROPDB_TARGETS = $(addsuffix _ropdb.txt, $(addprefix menu_ropdb/, $(ROPDB_VERSIONS)))

OUTNAME = $(FIRMVERSION)_$(REGION)_$(MENUVERSION)_$(MSETVERSION)

QRCODE_TARGET0	:=	q/$(OUTNAME).png
QRCODE_TARGET1	:=	build/cn_save_initial_loader.bin
QRCODE_TARGET1_CMD	:=	@cp $(QRCODE_TARGET1) cn_secondary_payload/data/

ifneq ($(strip $(OTHERAPP)),)
	PAYLOAD_SRCPATH	:=	cn_secondary_payload/cn_secondary_payload.bin
	QRCODE_TARGET0	:=	
	QRCODE_TARGET1	:=	
	QRCODE_TARGET1_CMD	:=	
endif

SCRIPTS = "scripts"

.PHONY: directories all menu_ropdb build/constants firm_constants/constants.txt cn_constants/constants.txt region_constants/constants.txt menu_ropdb/ropdb.txt cn_qr_initial_loader/cn_qr_initial_loader.bin.png cn_save_initial_loader/cn_save_initial_loader.bin cn_secondary_payload/cn_secondary_payload.bin cn_bootloader/cn_bootloader.bin menu_payload/menu_payload_regionfree.bin menu_payload/menu_payload_loadropbin.bin menu_payload/menu_ropbin.bin

all: directories build/constants $(QRCODE_TARGET0) p/$(OUTNAME).bin r/$(OUTNAME).bin $(QRCODE_TARGET1)
directories:
	@mkdir -p build && mkdir -p build/cro
	@mkdir -p p
	@mkdir -p q
	@mkdir -p r
	@mkdir -p qri

menu_ropdb: $(ROPDB_TARGETS)

menu_ropdb/%_ropdb.txt: menu_ropdb/17415_ropdb_proto.txt
	@echo building ropDB for menu version $*...
	@python scripts/portRopDb.py menu_17415_code.bin menu_$*_code.bin 0x00100000 menu_ropdb/17415_ropdb_proto.txt menu_ropdb/$*_ropdb.txt

q/$(OUTNAME).png: build/cn_qr_initial_loader.bin.png
	@cp build/cn_qr_initial_loader.bin.png q/$(OUTNAME).png

p/$(OUTNAME).bin: $(PAYLOAD_SRCPATH)
	@cp $(PAYLOAD_SRCPATH) p/$(OUTNAME).bin

r/$(OUTNAME).bin: menu_ropbin_patcher/menu_ropbin.exe menu_payload/menu_ropbin.bin build/menu_payload_loadropbin.bin
	@cd menu_ropbin_patcher && ./menu_ropbin.exe ../menu_payload/menu_ropbin.bin ../$@_
	@cat $@_ build/menu_payload_loadropbin.bin > $@
	@rm $@_

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

menu_ropbin_patcher/menu_ropbin.exe:
	@cd menu_ropbin_patcher && make

compress/compress.exe:
	@cd compress && make


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


app_bootloader/app_payload.bin: app_payload/app_payload.bin
	@mkdir -p app_bootloader/data/
	@cp app_payload/app_payload.bin app_bootloader/data/
app_payload/app_payload.bin:
	@cd app_payload && make


build/app_bootloader.bin: app_bootloader/app_bootloader.bin
	@cp app_bootloader/app_bootloader.bin build
app_bootloader/app_bootloader.bin: app_bootloader/app_payload.bin
	@cd app_bootloader && make


build/cn_secondary_payload.bin: cn_secondary_payload/cn_secondary_payload.bin
	@cp cn_secondary_payload/cn_secondary_payload.bin build/cn_secondary_payload.bin
	@$(SCRIPTS)/blz.exe -en build/cn_secondary_payload.bin
	@python $(SCRIPTS)/blowfish.py build/cn_secondary_payload.bin build/cn_secondary_payload.bin scripts
cn_secondary_payload/cn_secondary_payload.bin: build/cn_save_initial_loader.bin build/menu_payload_regionfree.bin build/menu_payload_loadropbin.bin build/menu_ropbin.bin compress/compress.exe
	@mkdir -p cn_secondary_payload/data
	@rm -rf cn_secondary_payload/data/*
ifeq ($(strip $(QRINSTALLER)),)
	@cp build/cn_save_initial_loader.bin cn_secondary_payload/data/
	@cp build/menu_payload_regionfree.bin cn_secondary_payload/data/
endif
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
	@cd app_payload && make clean
	@cd app_bootloader && make clean
	@cd app_code && make clean
	@cd menu_ropbin_patcher && make clean
	@echo "all cleaned up !"
