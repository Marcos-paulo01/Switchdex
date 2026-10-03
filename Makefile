#---------------------------------------------------------------------------------
# SwitchDex - Nintendo Switch Homebrew Makefile (devkitA64)
#---------------------------------------------------------------------------------
.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
endif

include $(DEVKITPRO)/libnx/switch_rules

TARGET      := switchdex
BUILD       := build
SOURCES     := source source/mount source/parser source/db source/ui
INCLUDES    := include include/db include/mount include/parser include/ui
ROMFS       := romfs

ARCH        := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

PREFIX      := aarch64-none-elf-
CC          := $(PREFIX)gcc
CXX         := $(PREFIX)g++
LD          := $(PREFIX)g++
ELF2NRO     := elf2nro
NACPTOOL    := nacptool

SQLITE_FLAGS := -DSQLITE_OMIT_WAL=1 -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_THREADSAFE=0 \
                 -DSQLITE_NO_SYNC=1 -DSQLITE_ENABLE_LOCKING_STYLE=0 \
                 -DHAVE_FCHOWN=0 -DHAVE_GETEUID=0 -DHAVE_UTIME=0

CFLAGS      := -g -Wall -O2 -ffunction-sections -fdata-sections $(ARCH) $(SQLITE_FLAGS) \
               -I$(PORTLIBS)/include -I$(LIBNX)/include $(foreach dir,$(INCLUDES),-I$(dir))

CXXFLAGS    := $(CFLAGS) -std=gnu++20 -fno-rtti -fno-exceptions

LIBPATHS    := -L$(PORTLIBS)/lib -L$(LIBNX)/lib
LIBS        := -specs=$(LIBNX)/switch.specs $(LIBPATHS) \
               -lSDL2_image -lSDL2_ttf -lSDL2 -lEGL -lGLESv2 -lglapi -ldrm_nouveau \
               -lwebp -lfreetype -lharfbuzz -lbz2 -lpng -ljpeg -lz -lnx -lm

CFILES      := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c))
CPPFILES    := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.cpp))

OBJS        := $(patsubst %.c,$(BUILD)/%.o,$(CFILES)) $(patsubst %.cpp,$(BUILD)/%.o,$(CPPFILES))

NACPFLAGS   := --name "SwitchDex" --author "Homebrew" --version "1.0.0"

ifneq ($(strip $(ROMFS)),)
    NROFLAGS := --romfsdir="$(ROMFS)"
endif

.PHONY: all clean

all: $(TARGET).nro

$(TARGET).nro: $(TARGET).elf $(TARGET).nacp
	@echo "[NRO]    $@"
	@$(ELF2NRO) $< $@ --nacp=$(TARGET).nacp $(NROFLAGS)

$(TARGET).nacp:
	@echo "[NACP]   $@"
	@$(NACPTOOL) --create "SwitchDex" "Homebrew" "1.0.0" $@

$(TARGET).elf: $(OBJS)
	@echo "[LINK]   $@"
	@$(LD) $(ARCH) -Wl,-Map,$(TARGET).map -Wl,--gc-sections $(OBJS) $(LIBS) -o $@

$(BUILD)/%.o: %.c
	@echo "[CC]     $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.cpp
	@echo "[CXX]    $<"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "[CLEAN]  Removendo arquivos compilados..."
	@rm -rf $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf $(TARGET).map
