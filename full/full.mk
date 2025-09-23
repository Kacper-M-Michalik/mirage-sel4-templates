### Build toolchain setup and Build settings ###
ifeq ($(ARCH),aarch64)
	TOOLCHAIN := aarch64-none-elf
else
$(error Unsupported ARCH)
endif

ifeq ($(strip $(MICROKIT_BOARD)), qemu_virt_aarch64)
	DRIVER_DIR := arm
	TIMER_DIR := arm
	CPU := cortex-a53
else
$(error Unsupported MICROKIT_BOARD)
endif

CC := $(TOOLCHAIN)-gcc
LD := $(TOOLCHAIN)-ld
AS := $(TOOLCHAIN)-as
AR := $(TOOLCHAIN)-ar
OBJCOPY := $(TOOLCHAIN)-objcopy
RANLIB := $(TOOLCHAIN)-ranlib
DTC := dtc
PYTHON ?= python3
MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit


### Dependencies and core settings ###
SOLO5LIBVMM := $(DEPS)/solo5libvmm
SDDF := $(DEPS)/sddf

SYS_GEN := $(realpath $(SRC_DIR)/system_gen.py)
DTS := $(SDDF)/dts/$(MICROKIT_BOARD).dts
DTB := $(MICROKIT_BOARD).dtb

IMAGES := vmm.elf serial_driver.elf serial_virt_tx.elf serial_virt_rx.elf timer_driver.elf
VMM_OBJS = vmm.o guest_img.o
SDDF_CUSTOM_LIBC := 1 

CFLAGS := -mstrict-align -nostdlib -ffreestanding -g -O3 -Wall -Wno-unused-function \
		-I$(BOARD_DIR)/include \
		-I$(SDDF)/include \
		-I$(SDDF)/include/microkit \
		-I$(SOLO5LIBVMM)/include \
		-I$(INC_DIR)

LDFLAGS := -L$(BOARD_DIR)/lib -L$(SDDF)/lib
LIBS := --start-group -lmicrokit -Tmicrokit.ld libsddf_util_debug.a solo5libvmm.a --end-group


### Build rules ###
$(IMAGES): libsddf_util_debug.a solo5libvmm.a

include $(SDDF)/util/util.mk
include $(SDDF)/drivers/timer/$(TIMER_DIR)/timer_driver.mk
include $(SDDF)/drivers/serial/$(DRIVER_DIR)/serial_driver.mk
include $(SDDF)/serial/components/serial_components.mk

include $(SOLO5LIBVMM)/solo5libvmm.mk

vmm.elf: $(VMM_OBJS) libsddf_util.a solo5libvmm.a
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

vmm.o: $(SRC_DIR)/vmm.c
	$(CC) $(CFLAGS) -c -o $@ $< 

guest_img.o: $(GUEST_FILE)
	cp $(GUEST_FILE) ./guest.hvt
	$(OBJCOPY) -I binary -O elf64-littleaarch64 -B aarch64 \
	--redefine-sym _binary_guest_hvt_start=_binary_guest_start \
	--redefine-sym _binary_guest_hvt_end=_binary_guest_end \
	--redefine-sym _binary_guest_hvt_size=_binary_guest_size \
	guest.hvt $@

$(DTB): $(DTS)
	dtc -q -I dts -O dtb $(DTS) > $(DTB)

#rm and cp are temporary until we fix system_gen
$(SYSTEM): $(SYS_GEN) $(IMAGES) $(DTB)
	$(PYTHON) $(SYS_GEN) --sddf $(SDDF) --board $(MICROKIT_BOARD) --dtb $(DTB) --output . --sdf $(SYSTEM)
	$(OBJCOPY) --update-section .device_resources=serial_driver_device_resources.data serial_driver.elf
	$(OBJCOPY) --update-section .serial_driver_config=serial_driver_config.data serial_driver.elf
	$(OBJCOPY) --update-section .serial_virt_rx_config=serial_virt_rx.data serial_virt_rx.elf
	$(OBJCOPY) --update-section .serial_virt_tx_config=serial_virt_tx.data serial_virt_tx.elf
	$(OBJCOPY) --update-section .device_resources=timer_driver_device_resources.data timer_driver.elf
	$(OBJCOPY) --update-section .timer_client_config=timer_client_VMM.data vmm.elf
	$(OBJCOPY) --update-section .serial_client_config=serial_client_VMM.data vmm.elf

$(IMAGE_FILE) $(REPORT_FILE): $(IMAGES) $(SYSTEM)
	$(MICROKIT_TOOL) $(SYSTEM) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)