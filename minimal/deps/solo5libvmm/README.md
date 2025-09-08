This library provides setup and processing functions to run solo5 HVT unikernels in Microkit PDs, you must provide your own implemenation of the hypercall handlers.
Example systems/PDs built using this library, plus a guide to building HVT unikernels (like with MirageOS) can be found here: https://github.com/Kacper-M-Michalik/Mirage-Sel4-Templates

This library depends on libmicrokit, you to providing printf and _assert_fail implementations, and can be used in two ways:
You can add the include/ and src/ folders into your project and write your own build system.
You can run 'make ...' to create a library, and simply link against it.