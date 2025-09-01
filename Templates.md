Use these as both examples and as ready templates for creating sel4 systems with solo5 based unikernels.
Target boards these can be built for vary with each template, read the associated markdown files.
For building, these tempaltes require you to download the Microkit SDK 2.0.1.

Available templates:
- minimal: Starts one unikernel, does not implement any hypercalls
- full: Starts one unikernel, with hypercall implementations using SDDF for serial and block.
- full linux: Starts one unikernel, with hypercall implementations using Linux
- shared linux:  Starts two unikernels, with hypercall implementations using Linux
- split linux:  Starts one unikernel, with hypercall implementations using Linux, except for serial, which runs using SDDF