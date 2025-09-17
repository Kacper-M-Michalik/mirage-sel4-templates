# MirageOS/sel4 System Templates
These templates provide working MirageOS/sel4 systems to learn from or use in production.
Targets vary by template, read the associated markdown files.
<br>
Available templates:
- minimal: Starts one unikernel, does not implement any hypercalls
- full: Starts one unikernel, with hypercall implementations using SDDF for serial and timer
- full linux: Starts one unikernel, with hypercall implementations using Linux
- shared linux:  Starts two unikernels, with hypercall implementations using Linux
- split linux:  Starts one unikernel, with hypercall implementations using Linux, except for serial, which runs as a sel4 component using SDDF (+ Virtio for linux)

## Dependencies
- All templates require you have a Aarch64 bare-metal toolchain in your PATH, downloads available from [ARM](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads). Example command to add the toolchain to you path (do not include the <>):
```bash
export PATH=</example/path/arm-gnu-toolchain-14.3.rel1-x86_64-aarch64-none-elf/bin>:$PATH
```
- QEMU is required to run the virtualised board build
- You need a copy of [Microkit-SDK-2.0.1](https://docs.sel4.systems/releases/microkit.html)
- All templates depend on other repositories, make sure to run:
```bash
git submodule update --init
```