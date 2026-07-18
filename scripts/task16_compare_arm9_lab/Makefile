.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "DEVKITARM is not set")
endif

NDS_NORMALIZE_DEVKIT_PATH = $(patsubst %/,%,$(patsubst c:/%,/c/%,$(patsubst C:/%,/c/%,$(subst \,/,$(strip $(1))))))
override DEVKITPRO := $(call NDS_NORMALIZE_DEVKIT_PATH,$(DEVKITPRO))
override DEVKITARM := $(call NDS_NORMALIZE_DEVKIT_PATH,$(DEVKITARM))

include $(DEVKITARM)/ds_rules

TARGET := task16-compare-arm9-lab
BUILD := build
SOURCES := source
INCLUDES :=

ARCH := -march=armv5te -mtune=arm946e-s -mthumb
CFLAGS := -g -Wall -Wextra -Werror -O2 -ffunction-sections -fdata-sections $(ARCH)
CFLAGS += $(INCLUDE) -DARM9
ASFLAGS := -g $(ARCH)
LDFLAGS := -specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map),--gc-sections
LIBS := -lnds9
LIBDIRS := $(LIBNDS)

NDS_TASK9_FLOAT_LIBGCC_SHA256 := c755adc33eca252260360327904591b8462cce5c25e48b0e881ac0b295953f48

ifneq ($(BUILD),$(notdir $(CURDIR)))

export LAB_ROOT := $(CURDIR)
export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
export OFILES := $(CFILES:.c=.o) $(SFILES:.s=.o) \
	libgcc_compare_golden.o libgcc_unordered_golden.o
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
	$(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export LD := $(CC)

.PHONY: all clean $(BUILD)

all: $(BUILD)

$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds $(TARGET).ds.gba

else

DEPENDS := $(OFILES:.o=.d)
NDS_TASK9_FLOAT_LIBGCC := $(shell $(CC) $(ARCH) -print-libgcc-file-name)
NDS_TASK9_FLOAT_AR := $(shell $(CC) -print-prog-name=ar)

.PHONY: all

all: $(OUTPUT).nds

$(OUTPUT).nds: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

libgcc_compare_golden.o libgcc_unordered_golden.o &: $(LAB_ROOT)/Makefile
	@echo "$(NDS_TASK9_FLOAT_LIBGCC_SHA256) *$(NDS_TASK9_FLOAT_LIBGCC)" | sha256sum -c -
	@mkdir -p .task16-compare-golden
	@cp "$(NDS_TASK9_FLOAT_LIBGCC)" .task16-compare-golden/libgcc.a
	@cd .task16-compare-golden && $(NDS_TASK9_FLOAT_AR) x libgcc.a _arm_cmpsf2.o _arm_unordsf2.o
	@$(OBJCOPY) \
		--redefine-sym __aeabi_fcmplt=__nds_task16_libgcc_fcmplt_golden \
		--redefine-sym __aeabi_fcmple=__nds_task16_libgcc_fcmple_golden \
		--redefine-sym __aeabi_fcmpge=__nds_task16_libgcc_fcmpge_golden \
		--redefine-sym __aeabi_fcmpgt=__nds_task16_libgcc_fcmpgt_golden \
		--rename-section .text=.itcm,alloc,load,readonly,code,contents \
		.task16-compare-golden/_arm_cmpsf2.o libgcc_compare_golden.o
	@$(OBJCOPY) \
		--redefine-sym __aeabi_fcmpun=__nds_task16_libgcc_fcmpun_golden \
		--rename-section .text=.itcm,alloc,load,readonly,code,contents \
		.task16-compare-golden/_arm_unordsf2.o libgcc_unordered_golden.o
	@rm -rf .task16-compare-golden

-include $(DEPENDS)

endif
