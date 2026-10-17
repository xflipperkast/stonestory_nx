.SUFFIXES:
.DEFAULT_GOAL := all
ifeq ($(strip $(DEVKITPRO)),)
$(error DEVKITPRO is not set; export DEVKITPRO=/opt/devkitpro)
endif
TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules
TARGET := stonestory_nx
BUILD := .cache/build
SOURCES := source
INCLUDES := include
ROMFS := .cache/build/romfs
APP_TITLE := Stone Story RPG
APP_AUTHOR := xFlippy
APP_VERSION := 1.0.0
ICON := icon.jpg
export SOURCE_DATE_EPOCH ?= 1792195200
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS := -g -Os -Wall -Wextra -Wno-unused-function -ffunction-sections -fdata-sections $(ARCH) -DSSRNX_HAVE_VORBIS
CFLAGS += $(INCLUDE) -D__SWITCH__ `$(PREFIX)pkg-config --cflags freetype2`
CFLAGS += -ffile-prefix-map=$(TOPDIR)=. -fmacro-prefix-map=$(TOPDIR)=.
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := -g $(ARCH)
LDFLAGS := -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,--gc-sections -Wl,-Map,$(TARGET).map
LDFLAGS += -Wl,--wrap=_malloc_r -Wl,--wrap=_free_r -Wl,--wrap=_realloc_r -Wl,--wrap=_calloc_r -Wl,--wrap=_memalign_r -Wl,--wrap=_malloc_usable_size_r -Wl,--wrap=__libnx_exception_entry
LDFLAGS += -flto=4
LIBS := -Wl,--start-group -lEGL -lGLESv2 -lglapi -ldrm_nouveau -lvorbisfile -lvorbis -logg -lfreetype -lpng16 -lbz2 -lcurl -lmbedtls -lmbedx509 -lmbedcrypto -lz -llzma -Wl,--end-group -lstdc++ -lnx -lm
LIBDIRS := $(TOPDIR)/.cache/mesa-switch $(TOPDIR)/.cache/curl-switch $(TOPDIR)/.cache/freetype-switch $(PORTLIBS) $(LIBNX)
ifneq ($(notdir $(BUILD)),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(BUILD)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES := main.c cursor.c runtime_android.c runtime_java.c runtime_jni.c runtime_java_util.c runtime_security.c runtime_network.c runtime_loader.c runtime_bionic.c runtime_storage.c runtime_graphics.c runtime_input.c runtime_assets.c runtime_audio.c runtime_diagnostics.c runtime_database.c
SFILES := runtime_asm.s
export LD := $(CC)
export OFILES := $(CFILES:.c=.o) $(SFILES:.s=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export NROFLAGS := --icon=$(CURDIR)/$(ICON) --nacp=$(CURDIR)/$(BUILD)/$(TARGET).nacp --romfsdir=$(CURDIR)/$(ROMFS)
export ROMFS_FILES := $(CURDIR)/$(ROMFS)/platform.zip
.PHONY: all dependencies clean check package
all: dependencies
dependencies:
	@bash tools/mesa/build_switch.sh
	@bash tools/libcurl/build_switch.sh
	@bash tools/freetype/build_switch.sh
	@mkdir -p $(BUILD)
	@python3 tools/pack_romfs.py romfs $(ROMFS)/platform.zip $(BUILD)/platform_files.inc
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
	@cp $(OUTPUT).nro $(CURDIR)/$(TARGET).nro
clean:
	rm -rf $(BUILD) $(TARGET).nro
check:
	@bash tests/run.sh
package: all
	@python3 tools/package_release.py
else
LDFLAGS += -Wl,--build-id=0x$(shell python3 $(TOPDIR)/tools/build_id.py)
DEPENDS := $(OFILES:.o=.d)
.PHONY: all
all: $(OUTPUT).nro
$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp $(ROMFS_FILES) $(TOPDIR)/Makefile $(TOPDIR)/$(ICON)
$(OUTPUT).nacp: $(TOPDIR)/Makefile
$(OUTPUT).elf: $(OFILES) $(TOPDIR)/.cache/mesa-switch/lib/libEGL.a $(TOPDIR)/.cache/curl-switch/lib/libcurl.a $(TOPDIR)/.cache/freetype-switch/lib/libfreetype.a $(TOPDIR)/tools/build_id.py
$(OFILES): $(TOPDIR)/Makefile
main.o: $(DEPSDIR)/platform_files.inc
runtime_java.o: $(DEPSDIR)/android_api.inc $(DEPSDIR)/core_library_dex.inc $(DEPSDIR)/aosp_libcore_dex.inc
runtime_assets.o: $(DEPSDIR)/framework_resources_data.inc
runtime_java_util.o: $(DEPSDIR)/packed_idn.inc $(DEPSDIR)/packed_norm.inc
$(DEPSDIR)/packed_idn.inc: $(TOPDIR)/assets/platform/java_idn_data.inc $(TOPDIR)/tools/pack_unicode.py
	python3 $(TOPDIR)/tools/pack_unicode.py $< $@ packed_idn
$(DEPSDIR)/packed_norm.inc: $(TOPDIR)/assets/platform/unicode_normalization_data.inc $(TOPDIR)/tools/pack_unicode.py
	python3 $(TOPDIR)/tools/pack_unicode.py $< $@ packed_norm
$(DEPSDIR)/android_api.inc: $(TOPDIR)/assets/android_api.txt $(TOPDIR)/tools/embed_data.py
	python3 $(TOPDIR)/tools/embed_data.py $< $@ --text-xz
$(DEPSDIR)/core_library_dex.inc: $(TOPDIR)/assets/core_library_dex.dex $(TOPDIR)/tools/embed_data.py
	python3 $(TOPDIR)/tools/embed_data.py $< $@ --bytes-xz
$(DEPSDIR)/aosp_libcore_dex.inc: $(TOPDIR)/assets/aosp_libcore_dex.dex $(TOPDIR)/tools/embed_data.py
	python3 $(TOPDIR)/tools/embed_data.py $< $@ --bytes-xz
$(DEPSDIR)/framework_resources_data.inc: $(TOPDIR)/assets/framework.arsc.zlib $(TOPDIR)/assets/framework_roots.json $(TOPDIR)/tools/trim_resources.py $(TOPDIR)/tools/embed_data.py
	python3 $(TOPDIR)/tools/trim_resources.py $< $(TOPDIR)/assets/framework_roots.json $(DEPSDIR)/framework-trimmed.zlib
	python3 $(TOPDIR)/tools/embed_data.py $(DEPSDIR)/framework-trimmed.zlib $@ --framework
runtime_database.o: CFLAGS += -w
-include $(DEPENDS)
endif
