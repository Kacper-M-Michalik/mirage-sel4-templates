BUILD_DIR ?= build

ifeq ($(strip $(MICROKIT_SDK)),)
$(error MICROKIT_SDK must be specified)
endif

ifeq ($(strip $(MICROKIT_BOARD)),)
$(error MICROKIT_BOARD must be specified)
endif

ifeq ($(strip $(MICROKIT_CONFIG)),)
$(error MICROKIT_CONFIG must be specified)
endif

# Make relevant paths absolute
override MICROKIT_SDK := $(abspath ${MICROKIT_SDK})
BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)
ARCH := ${shell grep 'CONFIG_SEL4_ARCH  ' $(BOARD_DIR)/include/kernel/gen_config.h | cut -d' ' -f4}
DEPS := $(abspath ./../minimal/deps)
RESOURCES := $(abspath ./)
INC_DIR := $(abspath ./)
SRC_DIR := $(abspath ./)

# Core build files
IMAGE_FILE ?= loader.img
REPORT_FILE ?= report.txt
BUILD_MAKE ?= full.mk
GUEST_FILE ?= hello.hvt


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
export GUEST_FILE


### Call actual build makefile ###
all: $(BUILD_DIR)/$(IMAGE_FILE) $(BUILD_DIR)/$(REPORT_FILE)

$(BUILD_DIR)/$(IMAGE_FILE) $(REPORT_FILE): $(BUILD_DIR)/Makefile FORCE
	${MAKE} -C ${BUILD_DIR} $(notdir $@)

${BUILD_DIR}/Makefile: $(BUILD_MAKE)
	mkdir -p ${BUILD_DIR}
	cp $(BUILD_MAKE) $@

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

FORCE: