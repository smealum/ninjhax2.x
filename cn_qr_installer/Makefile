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
NAME = cn_qr_installer

.PHONY: all

all: $(NAME).bin.png

%.bin.png: %.bin
	@python $(SCRIPTS)/obfuscator5000.py $<
	@python $(SCRIPTS)/crypt.py $< $(SCRIPTS)
	@$(SCRIPTS)/qrcode.exe -8 -o $@ < tmp
	@rm tmp

clean:
	@rm -f $(NAME).bin $(NAME).bin.png

-include $(DFILES)

$(NAME).bin: sploit_proto.bin sploit.s
	armips sploit.s
