# Build toolchain setup
ifeq ($(strip $(BUILD_DIR)),)
$(error BUILD_DIR must be specified)
endif

ifeq ($(strip $(MICROKIT_SDK)),)
$(error MICROKIT_SDK must be specified)
endif

ifeq ($(strip $(MICROKIT_BOARD)),)
$(error MICROKIT_BOARD must be specified)
endif

ifeq ($(strip $(MICROKIT_CONFIG)),)
$(error MICROKIT_CONFIG must be specified)
endif

BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
ARCH := ${shell grep 'CONFIG_SEL4_ARCH  ' $(BOARD_DIR)/include/kernel/gen_config.h | cut -d' ' -f4}

ifeq ($(ARCH),aarch64)
	TOOLCHAIN := aarch64-none-elf
	# No specific AArch64 flags
	CFLAGS_ARCH :=
else
$(error Unsupported ARCH)
endif

CC := $(TOOLCHAIN)-gcc
LD := $(TOOLCHAIN)-ld
AS := $(TOOLCHAIN)-as
OBJCOPY := $(TOOLCHAIN)-objcopy
MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

# Files to build
IMAGE_FILE = $(BUILD_DIR)/loader.img
REPORT_FILE = $(BUILD_DIR)/report.txt
SYSTEM := minimal.system
IMAGES := vmm.elf

RES_DIR = res
GUEST_FILE = guest.hvt

SRC_DIR := src
INC_DIR = include
VMM_PD_OBJS := vmm.o printf.o util.o guest.o

# Build settings
CFLAGS := -mstrict-align -nostdlib -ffreestanding -g -O3 -Wall -Wno-unused-function -I$(BOARD_DIR)/include
LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := -lmicrokit -Tmicrokit.ld

# Build
all: $(IMAGE_FILE)

# Compile all C files in src
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c Makefile
	$(CC) -c $(CFLAGS) -I$(INC_DIR) $< -o $@

# Link C files in build dir to create VMM PD elf
$(BUILD_DIR)/vmm.elf: $(addprefix $(BUILD_DIR)/, $(VMM_PD_OBJS))
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

# Special rule to convert guest image into .o file for linking as symbol
$(BUILD_DIR)/guest.o: $(RES_DIR)/$(GUEST_FILE)
	$(OBJCOPY) -I binary -O elf64-littleaarch64 -B aarch64 \
	--redefine-sym _binary_res_guest_hvt_start=_binary_guest_start \
	--redefine-sym _binary_res_guest_hvt_end=_binary_guest_end \
	--redefine-sym _binary_res_guest_hvt_size=_binary_guest_size \
	$< $@

# Build final image using Microkit_Tool
$(IMAGE_FILE) $(REPORT_FILE): $(addprefix $(BUILD_DIR)/, $(IMAGES)) $(SYSTEM)
	$(MICROKIT_TOOL) $(SYSTEM) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)