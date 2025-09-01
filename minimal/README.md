To build this example, fill in and run:
'''bash
mkdir build
make BUILD_DIR=build MICROKIT_BOARD=qemu_virt_aarch64 MICROKIT_CONFIG=<debug/release/benchmark> MICROKIT_SDK=/your/path/microkit-sdk-2.0.1
'''

Errors may be due to lack of a aarch64 cross-compiler in your path. Download a aarch64 bare-metal compiler toolchain here (make sure to pick your correct arch/OS):
https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads

To add the compilers to your path, you will a command similiar to this:
'''bash
export PATH=/example/path/arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-elf/bin:$PATH
'''

To simulate this example, run:
'''bash
qemu-system-aarch64 -machine virt,virtualization=on,highmem=off,secure=off \
				-cpu cortex-a53 \
				-serial mon:stdio \
				-device loader,file=build/loader.img,addr=0x70000000,cpu-num=0 \
				-m size=2G \
				-nographic
'''