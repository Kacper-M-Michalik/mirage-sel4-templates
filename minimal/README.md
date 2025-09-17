This example should be buildable for all aarch64 boards that support virtualisation (Armv8)

To build this example, fill in and run:
```bash
make MICROKIT_SDK=</your/path/microkit-sdk-2.0.1> MICROKIT_BOARD=<board> MICROKIT_CONFIG=<debug/release/benchmark> GUEST_FILE=<file>
```

To simulate this example, when building for qemu_virt_aarch64 add 'qemu' to your build command:
```bash
make qemu MICROKIT_SDK=</your/path/microkit-sdk-2.0.1> MICROKIT_BOARD=<board> MICROKIT_CONFIG=<debug/release/benchmark> GUEST_FILE=<file>
```

The resulting build files and final image (loader.img) can be seen in the 'build/' subfolder.