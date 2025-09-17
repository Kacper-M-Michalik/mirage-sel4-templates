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
AR := $(TOOLCHAIN)-ar
MICROKIT_TOOL := $(MICROKIT_SDK)/bin/microkit


### Dependencies and core settings ###
SOLO5LIBVMM := $(DEPS)/solo5libvmm
SDDF := $(DEPS)/sddf

IMAGES := vmm.elf
VMM_OBJS = vmm.o guest_img.o
SDDF_CUSTOM_LIBC := 1 
# We want to use the sddf standard library above for string/printf/assert implementations

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
include $(SOLO5LIBVMM)/solo5libvmm.mk

vmm.elf: $(VMM_OBJS) libsddf_util_debug.a solo5libvmm.a
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

vmm.o: $(SRC_DIR)/vmm.c
	$(CC) $(CFLAGS) -c -o $@ $< 

# Copy guest image into build dir and rename symbols for linking into vmm.c
guest_img.o: $(GUEST_FILE)
	cp $(GUEST_FILE) ./guest.hvt
	$(OBJCOPY) -I binary -O elf64-littleaarch64 -B aarch64 \
	--redefine-sym _binary_guest_hvt_start=_binary_guest_start \
	--redefine-sym _binary_guest_hvt_end=_binary_guest_end \
	--redefine-sym _binary_guest_hvt_size=_binary_guest_size \
	guest.hvt $@

# Generate final image
$(IMAGE_FILE) $(REPORT_FILE): $(IMAGES) $(SYSTEM)
	$(MICROKIT_TOOL) $(SYSTEM) --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(IMAGE_FILE) -r $(REPORT_FILE)