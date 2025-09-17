This example shows how you can implement a proper VMM that interacts with sel4 space hardware drivers to implement hypercalls, the SDDF project is used for driver/protocol implementation.
<br>
Additional details abotu SDDF can be found here:
- https://github.com/au-ts/sddf/tree/main
- https://trustworthy.systems/projects/drivers/sddf-design.pdf

The SDDF system results in complicated and fragile system descriptions, as such the sdfgen python library is used, which allows writing python to help automate generation of a XML system description, as seen in system_gen.py
<br>

This example is buildable for:
- qemu_virt_aarch64
- rpi4b_1gb #TBD
- rpi4b_2gb #TBD
- rpi4b_4gb #TBD
- rpi4b_8gb #TBD

Some precompiled MirageOS unikernels can be found in '/example_kernels' folder
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