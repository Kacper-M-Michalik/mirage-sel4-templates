### Dependencies and core settings ###
LIBVMM := $(DEPS)/solo5libvmm
SDDF := $(DEPS)/sddf

IMAGES := vmm.elf
GUEST_FILE := hello.hvt
# We want to use the sddf standard library implementation
SDDF_CUSTOM_LIBC := 1 

### Build toolchain setup and Build settings ###
ifeq ($(ARCH),aarch64)
	TOOLCHAIN := aarch64-none-elf
else
$(error Unsupported ARCH)
endif

CC := $(TOOLCHAIN)-gcc
LD := $(TOOLCHAIN)-ld
AS := $(TOOLCHAIN)-as
OBJCOPY := $(TOOLCHAIN)-objcopy
RANLIB := $(TOOLCHAIN)-ranlib
MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

CFLAGS := -mstrict-align -nostdlib -ffreestanding -g -O3 -Wall -Wno-unused-function \
		-I$(BOARD_DIR)/include \
		-I$(SDDF)/include \
		-I$(SDDF)/include/microkit \
		-I$(LIBVMM)/include \
		-I$(INC_DIR)

LDFLAGS := -L$(BOARD_DIR)/lib -L$(SDDF)/lib
LIBS := --start-group -lmicrokit -Tmicrokit.ld libsddf_util_debug.a --end-group

### Build rules ###
$(IMAGES): libsddf_util_debug.a

include $(SDDF)/util/util.mk

# TEMP
vmm.elf: vmm.o elf_solo5.o guest.o vcpu_aarch64.o guest_img.o libsddf_util_debug.a
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

vmm.o: $(SRC_DIR)/vmm.c
	$(CC) -c $(CFLAGS) $< -o $@

# TEMP RULE FOR BUILDING SOLO5LIBVMM
%.o: $(LIBVMM)/src/%.c
	$(CC) -c $(CFLAGS) $< -o $@

guest_img.o: $(RESOURCES)/$(GUEST_FILE) Makefile
	cp $(RESOURCES)/$(GUEST_FILE) ./guest.hvt
	$(OBJCOPY) -I binary -O elf64-littleaarch64 -B aarch64 \
	--redefine-sym _binary_guest_hvt_start=_binary_guest_start \
	--redefine-sym _binary_guest_hvt_end=_binary_guest_end \
	--redefine-sym _binary_guest_hvt_size=_binary_guest_size \
	guest.hvt $@

$(IMAGE_FILE) $(REPORT_FILE): $(IMAGES) $(SYSTEM) Makefile
	$(MICROKIT_TOOL) $(SYSTEM) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)