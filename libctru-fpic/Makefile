CC = C:/devkitPro/devkitARM/bin/arm-none-eabi-gcc
AR = C:/devkitPro/devkitARM/bin/arm-none-eabi-ar
CFLAGS += -Wall -std=c99 -march=armv6 -Os -fpic -I"$(CURDIR)/include/"

CFILES = $(wildcard source/*.c)
OFILES = $(CFILES:source/%.c=build/%.o)
DFILES = $(CFILES:source/%.c=build/%.d)
SFILES = $(wildcard source/*.s)
OFILES += $(SFILES:source/%.s=build/%.o)
PROJECTNAME = "libctru"

.PHONY:=all dir

all: dir lib/$(PROJECTNAME).a

dir:
	mkdir -p build
	mkdir -p lib

lib/$(PROJECTNAME).a: $(OFILES)
	$(AR) rvs $@ $^

clean:
	@rm -f build/*.o build/*.d
	@rm -f $(PROJECTNAME).a
	@echo "all cleaned up !"

-include $(DFILES)

build/%.o: source/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) -MM $< > build/$*.d

build/%.o: source/%.s
	$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) -MM $< > build/$*.d
