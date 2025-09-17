This example should be buildable for all aarch64 boards that support virtualisation (Armv8)

To build this example, fill in and run:
```bash
make BUILD_DIR=build MICROKIT_BOARD=<board> MICROKIT_CONFIG=<debug/release/benchmark> MICROKIT_SDK=</your/path/microkit-sdk-2.0.1> GUEST_FILE=<file>
```

To simulate this example, when building for qemu_virt_aarch64 add 'qemu' to your build command:
```bash
make qemu BUILD_DIR=build MICROKIT_BOARD=<board> MICROKIT_CONFIG=<debug/release/benchmark> MICROKIT_SDK=</your/path/microkit-sdk-2.0.1> GUEST_FILE=<file>
```