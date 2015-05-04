ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

ifeq ($(strip $(CTRULIB)),)
$(error "Please set CTRULIB in your environment. export DEVKITARM=<path to>ctrulib/libctru")
endif

ifeq ($(filter $(DEVKITARM)/bin,$(PATH)),)
export PATH:=$(DEVKITARM)/bin:$(PATH)
endif

SCRIPTS = "../scripts"

all: menu_payload.bin

clean:
	@rm -f menu_payload.bin
	@cd 12288 && make clean
	@cd 17415 && make clean
	@echo "all cleaned up !"

menu_payload.bin:
	@cd 17415 && make
	@cp 17415/$@ ./
