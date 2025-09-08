To build this example, fill in and run:
'''bash
mkdir build
make BUILD_DIR=build MICROKIT_BOARD=<board> MICROKIT_CONFIG=<debug/release/benchmark> MICROKIT_SDK=/your/path/microkit-sdk-2.0.1 GUEST_FILE=<file>
'''

To simulate this example, when building for qemu_virt_aarch64 add 'qemu' to your build command, or run:
'''bash
qemu-system-aarch64 -machine virt,virtualization=on,highmem=off,secure=off \
				-cpu cortex-a53 \
				-serial mon:stdio \
				-device loader,file=build/loader.img,addr=0x70000000,cpu-num=0 \
				-m size=2G \
				-nographic
'''


