ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

ifeq ($(strip $(CTRULIB)),)
$(error "Please set CTRULIB in your environment. export DEVKITARM=<path to>ctrulib/libctru")
endif

ifeq ($(filter $(DEVKITARM)/bin,$(PATH)),)
export PATH:=$(DEVKITARM)/bin:$(PATH)
endif

ROPBIN_CMD	:=	
ifneq ($(strip $(LOADROPBIN)),)
	ROPBIN_CMD	:=	@cp $(MENUVERSION)/menu_ropbin.bin ./
endif

SCRIPTS = "../scripts"

all: menu_payload_regionfree.bin menu_payload_loadropbin.bin menu_ropbin.bin

clean:
	@rm -f menu_payload_regionfree.bin menu_payload_loadropbin.bin menu_ropbin.bin
	@cd 12288 && make clean
	@cd 17415 && make clean
	@echo "all cleaned up !"

menu_payload_regionfree.bin menu_payload_loadropbin.bin menu_ropbin.bin:
	@cd $(MENUVERSION) && make
	@cp $(MENUVERSION)/menu_payload_regionfree.bin ./
	@cp $(MENUVERSION)/menu_payload_loadropbin.bin ./
	$(ROPBIN_CMD)
