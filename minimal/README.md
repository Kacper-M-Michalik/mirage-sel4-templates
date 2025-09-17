This example should be buildable for all aarch64 boards that support virtualisation (Armv8) and QEMU.
MICROKIT_BOARD options:
- qemu_virt_aarch64
- rpi4b_1gb
- rpi4b_2gb
- rpi4b_4gb
- rpi4b_8gb

Some precompiled MirageOS unikernels can be found in /examples
<br>

To build this example, fill in and run:
```bash
make MICROKIT_SDK=</your/path/microkit-sdk-2.0.1> MICROKIT_BOARD=<board> MICROKIT_CONFIG=<debug/release/benchmark> GUEST_FILE=<file>
```

This example can be simulated on QEMU, either run QEMU after building for qemu_virt_aarch64, or add 'qemu' to your build command:
```bash
make qemu MICROKIT_SDK=</your/path/microkit-sdk-2.0.1> MICROKIT_BOARD=<board> MICROKIT_CONFIG=<debug/release/benchmark> GUEST_FILE=<file>
```

The resulting build files and final image (loader.img) can be seen in the 'build/' subfolder.