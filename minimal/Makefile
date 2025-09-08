### Arg checks ###
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

# Files to build
IMAGE_FILE := loader.img
REPORT_FILE := report.txt
BUILD_MAKE := minimal.mk

# Make relevant paths absolute and export
override MICROKIT_SDK := $(abspath ${MICROKIT_SDK})
BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
ARCH := ${shell grep 'CONFIG_SEL4_ARCH  ' $(BOARD_DIR)/include/kernel/gen_config.h | cut -d' ' -f4}

DEPS := $(abspath ./deps)
RESOURCES := $(abspath ./res)
INC_DIR := $(abspath ./include)
SRC_DIR := $(abspath ./src)
SYSTEM := $(RESOURCES)/minimal.system

ifeq ($(strip $(GUEST_FILE)),)
GUEST_FILE := hello.hvt
endif

export BUILD_DIR
export MICROKIT_SDK
export MICROKIT_BOARD
export MICROKIT_CONFIG
export BOARD_DIR
export ARCH
export IMAGE_FILE
export REPORT_FILE
export INC_DIR
export SRC_DIR
export DEPS
export RESOURCES
export SYSTEM
export GUEST_FILE

all: $(BUILD_DIR)/$(IMAGE_FILE) $(BUILD_DIR)/$(REPORT_FILE)

### Call actual build makefile ###
$(BUILD_DIR)/$(IMAGE_FILE) $(REPORT_FILE): $(BUILD_DIR)/Makefile FORCE
	${MAKE} -C ${BUILD_DIR} $(notdir $@)

${BUILD_DIR}/Makefile: $(BUILD_MAKE)
	mkdir -p ${BUILD_DIR}
	cp $(BUILD_MAKE) $@

# Only allow qemu when built for virt_arch
ifeq ($(strip $(MICROKIT_BOARD)), qemu_virt_aarch64)
qemu: $(BUILD_DIR)/$(IMAGE_FILE) $(BUILD_DIR)/$(REPORT_FILE)
	qemu-system-aarch64 -machine virt,virtualization=on,highmem=off,secure=off \
				-cpu cortex-a53 \
				-serial mon:stdio \
				-device loader,file=$(BUILD_DIR)/$(IMAGE_FILE),addr=0x70000000,cpu-num=0 \
				-m size=2G \
				-nographic
else
qemu: $(BUILD_DIR)/$(IMAGE_FILE) $(BUILD_DIR)/$(REPORT_FILE)
	echo "QEMU not supported for $(MICROKIT_BOARD) (skipping simulation)"
endif 

# Used to always force rule to run
FORCE: